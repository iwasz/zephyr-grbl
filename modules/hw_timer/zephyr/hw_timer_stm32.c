/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_hw_timer

#include <errno.h>
#include <soc.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_tim.h>
#include "hw_timer.h"
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(hw_timer_stm32, CONFIG_HW_TIMER_LOG_LEVEL);

/* L0 series MCUs only have 16-bit timers and don't have below macro defined */
#ifndef IS_TIM_32B_COUNTER_INSTANCE
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) (0)
#endif

#ifdef CONFIG_HW_TIMER_CAPTURE
struct hw_timer_stm32_capture_data {
	hw_timer_capture_callback_handler_t callback;
	void *user_data;
	uint32_t period;
	uint32_t pulse;
	uint32_t overflows;
	uint8_t skip_irq;
	bool capture_period;
	bool capture_pulse;
	bool continuous;
};

/* first capture is always nonsense, second is nonsense when polarity changed */
#define SKIPPED_HW_TIMER_CAPTURES 2u

#endif /*CONFIG_HW_TIMER_CAPTURE*/

/** HW_TIMER data. */
struct hw_timer_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
#ifdef CONFIG_HW_TIMER_CAPTURE
	struct hw_timer_stm32_capture_data capture;
#endif /* CONFIG_HW_TIMER_CAPTURE */
	timer_callback_t timer_up_callback;
};

/** HW_TIMER configuration. */
struct hw_timer_stm32_config {
	TIM_TypeDef *timer;
	uint32_t prescaler;
	uint32_t countermode;
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_HW_TIMER_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_HW_TIMER_CAPTURE */
};

/** Maximum number of timer channels : some stm32 soc have 6 else only 4 */
#if defined(LL_TIM_CHANNEL_CH6)
#define TIMER_HAS_6CH 1
#define TIMER_MAX_CH 6u
#else
#define TIMER_HAS_6CH 0
#define TIMER_MAX_CH 4u
#endif

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = { LL_TIM_CHANNEL_CH1, LL_TIM_CHANNEL_CH2,
					      LL_TIM_CHANNEL_CH3, LL_TIM_CHANNEL_CH4,
#if TIMER_HAS_6CH
					      LL_TIM_CHANNEL_CH5, LL_TIM_CHANNEL_CH6
#endif
};

/** Some stm32 mcus have complementary channels : 3 or 4 */
static const uint32_t ch2ll_n[] = {
#if defined(LL_TIM_CHANNEL_CH1N)
	LL_TIM_CHANNEL_CH1N,
	LL_TIM_CHANNEL_CH2N,
	LL_TIM_CHANNEL_CH3N,
#if defined(LL_TIM_CHANNEL_CH4N)
	/** stm32g4x and stm32u5x have 4 complementary channels */
	LL_TIM_CHANNEL_CH4N,
#endif /* LL_TIM_CHANNEL_CH4N */
#endif /* LL_TIM_CHANNEL_CH1N */
};
/** Maximum number of complemented timer channels is ARRAY_SIZE(ch2ll_n)*/

/** Channel to compare set function mapping. */
static void (*const set_timer_compare[TIMER_MAX_CH])(TIM_TypeDef *, uint32_t) = {
	LL_TIM_OC_SetCompareCH1, LL_TIM_OC_SetCompareCH2,
	LL_TIM_OC_SetCompareCH3, LL_TIM_OC_SetCompareCH4,
#if TIMER_HAS_6CH
	LL_TIM_OC_SetCompareCH5, LL_TIM_OC_SetCompareCH6
#endif
};

/**
 * Obtain LL polarity from HW_TIMER flags.
 *
 * @param flags HW_TIMER flags.
 *
 * @return LL polarity.
 */
static uint32_t get_polarity(hw_timer_flags_t flags)
{
	if ((flags & HW_TIMER_POLARITY_MASK) == HW_TIMER_POLARITY_NORMAL) {
		return LL_TIM_OCPOLARITY_HIGH;
	}

	return LL_TIM_OCPOLARITY_LOW;
}

/**
 * @brief  Check if LL counter mode is center-aligned.
 *
 * @param  ll_countermode LL counter mode.
 *
 * @return `true` when center-aligned, otherwise `false`.
 */
static inline bool is_center_aligned(const uint32_t ll_countermode)
{
	return ((ll_countermode == LL_TIM_COUNTERMODE_CENTER_DOWN) ||
		(ll_countermode == LL_TIM_COUNTERMODE_CENTER_UP) ||
		(ll_countermode == LL_TIM_COUNTERMODE_CENTER_UP_DOWN));
}

/**
 * Obtain timer clock speed.
 *
 * @param pclken  Timer clock control subsystem.
 * @param tim_clk Where computed timer clock will be stored.
 *
 * @return 0 on success, error code otherwise.
 */
static int get_tim_clk(const struct stm32_pclken *pclken, uint32_t *tim_clk)
{
	int r;
	const struct device *clk;
	uint32_t bus_clk, apb_psc;

	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	r = clock_control_get_rate(clk, (clock_control_subsys_t)pclken, &bus_clk);
	if (r < 0) {
		return r;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = STM32_D2PPRE1;
	} else {
		apb_psc = STM32_D2PPRE2;
	}
#else
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = STM32_APB1_PRESCALER;
	}
#if !defined(CONFIG_SOC_SERIES_STM32C0X) && !defined(CONFIG_SOC_SERIES_STM32F0X) &&                \
	!defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
		apb_psc = STM32_APB2_PRESCALER;
	}
#endif
#endif

#if defined(RCC_DCKCFGR_TIMPRE) || defined(RCC_DCKCFGR1_TIMPRE) || defined(RCC_CFGR_TIMPRE)
	/*
	 * There are certain series (some F4, F7 and H7) that have the TIMPRE
	 * bit to control the clock frequency of all the timers connected to
	 * APB1 and APB2 domains.
	 *
	 * Up to a certain threshold value of APB{1,2} prescaler, timer clock
	 * equals to HCLK. This threshold value depends on TIMPRE setting
	 * (2 if TIMPRE=0, 4 if TIMPRE=1). Above threshold, timer clock is set
	 * to a multiple of the APB domain clock PCLK{1,2} (2 if TIMPRE=0, 4 if
	 * TIMPRE=1).
	 */

	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
		/* TIMPRE = 0 */
		if (apb_psc <= 2u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 2u;
		}
	} else {
		/* TIMPRE = 1 */
		if (apb_psc <= 4u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 4u;
		}
	}
#else
	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1u) {
		*tim_clk = bus_clk;
	} else {
		*tim_clk = bus_clk * 2u;
	}
#endif

	return 0;
}

static int hw_timer_stm32_set_cycles(const struct device *dev, uint32_t channel,
				     uint32_t period_cycles, uint32_t pulse_cycles,
				     hw_timer_flags_t flags)
{
	const struct hw_timer_stm32_config *cfg = dev->config;

	uint32_t ll_channel;
	uint32_t current_ll_channel; /* complementary output if used */
	uint32_t negative_ll_channel;

	if (channel < 1u || channel > TIMER_MAX_CH) {
		LOG_ERR("Invalid channel (%d)", channel);
		return -EINVAL;
	}

	/*
	 * Non 32-bit timers count from 0 up to the value in the ARR register
	 * (16-bit). Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!IS_TIM_32B_COUNTER_INSTANCE(cfg->timer) && (period_cycles > UINT16_MAX + 1)) {
		return -ENOTSUP;
	}

#ifdef CONFIG_HW_TIMER_CAPTURE
	if ((channel == 1u) || (channel == 2u)) {
		if (LL_TIM_IsEnabledIT_CC1(cfg->timer) || LL_TIM_IsEnabledIT_CC2(cfg->timer)) {
			LOG_ERR("Cannot set HW_TIMER output, capture in progress");
			return -EBUSY;
		}
	}
#endif /* CONFIG_HW_TIMER_CAPTURE */

	ll_channel = ch2ll[channel - 1u];

	if (channel <= ARRAY_SIZE(ch2ll_n)) {
		negative_ll_channel = ch2ll_n[channel - 1u];
	} else {
		negative_ll_channel = 0;
	}

	/* in LL_TIM_CC_DisableChannel and LL_TIM_CC_IsEnabledChannel,
	 * the channel param could be the complementary one
	 */
	if ((flags & STM32_HW_TIMER_COMPLEMENTARY_MASK) == STM32_HW_TIMER_COMPLEMENTARY) {
		if (!negative_ll_channel) {
			/* setting a flag on a channel that has not this capability */
			LOG_ERR("Channel %d has NO complementary output", channel);
			return -EINVAL;
		}
		current_ll_channel = negative_ll_channel;
	} else {
		current_ll_channel = ll_channel;
	}

	if (period_cycles == 0u) {
		LL_TIM_CC_DisableChannel(cfg->timer, current_ll_channel);
		return 0;
	}

	if (cfg->countermode == LL_TIM_COUNTERMODE_UP) {
		/* remove 1 period cycle, accounts for 1 extra low cycle */
		period_cycles -= 1U;
	} else if (cfg->countermode == LL_TIM_COUNTERMODE_DOWN) {
		/* remove 1 pulse cycle, accounts for 1 extra high cycle */
		pulse_cycles -= 1U;
		/* remove 1 period cycle, accounts for 1 extra low cycle */
		period_cycles -= 1U;
	} else if (is_center_aligned(cfg->countermode)) {
		pulse_cycles /= 2U;
		period_cycles /= 2U;
	} else {
		return -ENOTSUP;
	}

	if (!LL_TIM_CC_IsEnabledChannel(cfg->timer, current_ll_channel)) {
		LL_TIM_OC_InitTypeDef oc_init;

		LL_TIM_OC_StructInit(&oc_init);

		oc_init.OCMode = LL_TIM_OCMODE_PWM1;

#if defined(LL_TIM_CHANNEL_CH1N)
		/* the flags holds the STM32_HW_TIMER_COMPLEMENTARY information */
		if ((flags & STM32_HW_TIMER_COMPLEMENTARY_MASK) == STM32_HW_TIMER_COMPLEMENTARY) {
			oc_init.OCNState = LL_TIM_OCSTATE_ENABLE;
			oc_init.OCNPolarity = get_polarity(flags);

			/* inherit the polarity of the positive output */
			oc_init.OCState = LL_TIM_CC_IsEnabledChannel(cfg->timer, ll_channel) ?
						  LL_TIM_OCSTATE_ENABLE :
						  LL_TIM_OCSTATE_DISABLE;
			oc_init.OCPolarity = LL_TIM_OC_GetPolarity(cfg->timer, ll_channel);
		} else {
			oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
			oc_init.OCPolarity = get_polarity(flags);

			/* inherit the polarity of the negative output */
			if (negative_ll_channel) {
				oc_init.OCNState = LL_TIM_CC_IsEnabledChannel(cfg->timer,
									      negative_ll_channel) ?
							   LL_TIM_OCSTATE_ENABLE :
							   LL_TIM_OCSTATE_DISABLE;
				oc_init.OCNPolarity =
					LL_TIM_OC_GetPolarity(cfg->timer, negative_ll_channel);
			}
		}
#else /* LL_TIM_CHANNEL_CH1N */

		oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
		oc_init.OCPolarity = get_polarity(flags);
#endif /* LL_TIM_CHANNEL_CH1N */
		oc_init.CompareValue = pulse_cycles;

#ifdef CONFIG_HW_TIMER_CAPTURE
		if (IS_TIM_SLAVE_INSTANCE(cfg->timer)) {
			LL_TIM_SetSlaveMode(cfg->timer, LL_TIM_SLAVEMODE_DISABLED);
			LL_TIM_SetTriggerInput(cfg->timer, LL_TIM_TS_ITR0);
			LL_TIM_DisableMasterSlaveMode(cfg->timer);
		}
#endif /* CONFIG_HW_TIMER_CAPTURE */

		/* in LL_TIM_OC_Init, the channel is always the non-complementary */
		if (LL_TIM_OC_Init(cfg->timer, ll_channel, &oc_init) != SUCCESS) {
			LOG_ERR("Could not initialize timer channel output");
			return -EIO;
		}

		LL_TIM_EnableARRPreload(cfg->timer);
		/* in LL_TIM_OC_EnablePreload, the channel is always the non-complementary */
		LL_TIM_OC_EnablePreload(cfg->timer, ll_channel);
		LL_TIM_SetAutoReload(cfg->timer, period_cycles);
		LL_TIM_GenerateEvent_UPDATE(cfg->timer);
	} else {
		/* in LL_TIM_OC_SetPolarity, the channel could be the complementary one */
		LL_TIM_OC_SetPolarity(cfg->timer, current_ll_channel, get_polarity(flags));
		set_timer_compare[channel - 1u](cfg->timer, pulse_cycles);
		LL_TIM_SetAutoReload(cfg->timer, period_cycles);
	}

	return 0;
}

#ifdef CONFIG_HW_TIMER_CAPTURE
static int init_capture_channel(const struct device *dev, uint32_t channel, hw_timer_flags_t flags,
				uint32_t ll_channel)
{
	const struct hw_timer_stm32_config *cfg = dev->config;
	bool is_inverted = (flags & HW_TIMER_POLARITY_MASK) == HW_TIMER_POLARITY_INVERTED;
	LL_TIM_IC_InitTypeDef ic;

	LL_TIM_IC_StructInit(&ic);
	ic.ICPrescaler = TIM_ICPSC_DIV1;
	ic.ICFilter = LL_TIM_IC_FILTER_FDIV1;

	if (ll_channel == LL_TIM_CHANNEL_CH1) {
		if (channel == 1u) {
			ic.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
			ic.ICPolarity = is_inverted ? LL_TIM_IC_POLARITY_FALLING :
						      LL_TIM_IC_POLARITY_RISING;
		} else {
			ic.ICActiveInput = LL_TIM_ACTIVEINPUT_INDIRECTTI;
			ic.ICPolarity = is_inverted ? LL_TIM_IC_POLARITY_RISING :
						      LL_TIM_IC_POLARITY_FALLING;
		}
	} else {
		if (channel == 1u) {
			ic.ICActiveInput = LL_TIM_ACTIVEINPUT_INDIRECTTI;
			ic.ICPolarity = is_inverted ? LL_TIM_IC_POLARITY_RISING :
						      LL_TIM_IC_POLARITY_FALLING;
		} else {
			ic.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
			ic.ICPolarity = is_inverted ? LL_TIM_IC_POLARITY_FALLING :
						      LL_TIM_IC_POLARITY_RISING;
		}
	}

	if (LL_TIM_IC_Init(cfg->timer, ll_channel, &ic) != SUCCESS) {
		LOG_ERR("Could not initialize channel for HW_TIMER capture");
		return -EIO;
	}

	return 0;
}

static int hw_timer_stm32_configure_capture(const struct device *dev, uint32_t channel,
					    hw_timer_flags_t flags,
					    hw_timer_capture_callback_handler_t cb, void *user_data)
{
	/*
	 * Capture is implemented using the slave mode controller.
	 * This allows for high accuracy, but only CH1 and CH2 are supported.
	 * Alternatively all channels could be supported with ISR based resets.
	 * This is currently not implemented!
	 */

	const struct hw_timer_stm32_config *cfg = dev->config;
	struct hw_timer_stm32_data *data = dev->data;
	struct hw_timer_stm32_capture_data *cpt = &data->capture;
	int ret;

	if ((channel != 1u) && (channel != 2u)) {
		LOG_ERR("HW_TIMER capture only supported on first two channels");
		return -ENOTSUP;
	}

	if (LL_TIM_IsEnabledIT_CC1(cfg->timer) || LL_TIM_IsEnabledIT_CC2(cfg->timer)) {
		LOG_ERR("HW_TIMER Capture already in progress");
		return -EBUSY;
	}

	if (!(flags & HW_TIMER_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No HW_TIMER capture type specified");
		return -EINVAL;
	}

	if (!IS_TIM_SLAVE_INSTANCE(cfg->timer)) {
		LOG_ERR("Timer does not support slave mode for HW_TIMER capture");
		return -ENOTSUP;
	}

	cpt->callback = cb; /* even if the cb is reset, this is not an error */
	cpt->user_data = user_data;
	cpt->capture_period = (flags & HW_TIMER_CAPTURE_TYPE_PERIOD) ? true : false;
	cpt->capture_pulse = (flags & HW_TIMER_CAPTURE_TYPE_PULSE) ? true : false;
	cpt->continuous = (flags & HW_TIMER_CAPTURE_MODE_CONTINUOUS) ? true : false;

	/* Prevents faulty behavior while making changes */
	LL_TIM_SetSlaveMode(cfg->timer, LL_TIM_SLAVEMODE_DISABLED);

	ret = init_capture_channel(dev, channel, flags, LL_TIM_CHANNEL_CH1);
	if (ret < 0) {
		return ret;
	}

	ret = init_capture_channel(dev, channel, flags, LL_TIM_CHANNEL_CH2);
	if (ret < 0) {
		return ret;
	}

	if (channel == 1u) {
		LL_TIM_SetTriggerInput(cfg->timer, LL_TIM_TS_TI1FP1);
	} else {
		LL_TIM_SetTriggerInput(cfg->timer, LL_TIM_TS_TI2FP2);
	}
	LL_TIM_SetSlaveMode(cfg->timer, LL_TIM_SLAVEMODE_RESET);

	LL_TIM_EnableARRPreload(cfg->timer);
	if (!IS_TIM_32B_COUNTER_INSTANCE(cfg->timer)) {
		LL_TIM_SetAutoReload(cfg->timer, 0xffffu);
	} else {
		LL_TIM_SetAutoReload(cfg->timer, 0xffffffffu);
	}
	LL_TIM_EnableUpdateEvent(cfg->timer);

	return 0;
}

static int hw_timer_stm32_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct hw_timer_stm32_config *cfg = dev->config;
	struct hw_timer_stm32_data *data = dev->data;

	if ((channel != 1u) && (channel != 2u)) {
		LOG_ERR("HW_TIMER capture only supported on first two channels");
		return -EINVAL;
	}

	if (LL_TIM_IsEnabledIT_CC1(cfg->timer) || LL_TIM_IsEnabledIT_CC2(cfg->timer)) {
		LOG_ERR("HW_TIMER capture already active");
		return -EBUSY;
	}

	if (!data->capture.callback) {
		LOG_ERR("HW_TIMER capture not configured");
		return -EINVAL;
	}

	data->capture.skip_irq = SKIPPED_HW_TIMER_CAPTURES;
	data->capture.overflows = 0u;
	LL_TIM_ClearFlag_CC1(cfg->timer);
	LL_TIM_ClearFlag_CC2(cfg->timer);
	LL_TIM_ClearFlag_UPDATE(cfg->timer);

	LL_TIM_SetUpdateSource(cfg->timer, LL_TIM_UPDATESOURCE_COUNTER);
	if (channel == 1u) {
		LL_TIM_EnableIT_CC1(cfg->timer);
	} else {
		LL_TIM_EnableIT_CC2(cfg->timer);
	}
	LL_TIM_EnableIT_UPDATE(cfg->timer);
	LL_TIM_CC_EnableChannel(cfg->timer, LL_TIM_CHANNEL_CH1);
	LL_TIM_CC_EnableChannel(cfg->timer, LL_TIM_CHANNEL_CH2);

	return 0;
}

static int hw_timer_stm32_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct hw_timer_stm32_config *cfg = dev->config;

	if ((channel != 1u) && (channel != 2u)) {
		LOG_ERR("HW_TIMER capture only supported on first two channels");
		return -EINVAL;
	}

	LL_TIM_SetUpdateSource(cfg->timer, LL_TIM_UPDATESOURCE_REGULAR);
	if (channel == 1u) {
		LL_TIM_DisableIT_CC1(cfg->timer);
	} else {
		LL_TIM_DisableIT_CC2(cfg->timer);
	}
	LL_TIM_DisableIT_UPDATE(cfg->timer);
	LL_TIM_CC_DisableChannel(cfg->timer, LL_TIM_CHANNEL_CH1);
	LL_TIM_CC_DisableChannel(cfg->timer, LL_TIM_CHANNEL_CH2);

	return 0;
}

static void get_hw_timer_capture(const struct device *dev, uint32_t channel)
{
	const struct hw_timer_stm32_config *cfg = dev->config;
	struct hw_timer_stm32_data *data = dev->data;
	struct hw_timer_stm32_capture_data *cpt = &data->capture;

	if (channel == 1u) {
		cpt->period = LL_TIM_IC_GetCaptureCH1(cfg->timer);
		cpt->pulse = LL_TIM_IC_GetCaptureCH2(cfg->timer);
	} else {
		cpt->period = LL_TIM_IC_GetCaptureCH2(cfg->timer);
		cpt->pulse = LL_TIM_IC_GetCaptureCH1(cfg->timer);
	}
}

static void hw_timer_stm32_isr(const struct device *dev)
{
	const struct hw_timer_stm32_config *cfg = dev->config;
	struct hw_timer_stm32_data *data = dev->data;
	struct hw_timer_stm32_capture_data *cpt = &data->capture;
	int status = 0;
	uint32_t in_ch = LL_TIM_IsEnabledIT_CC1(cfg->timer) ? 1u : 2u;

	if (cpt->skip_irq == 0u) {
		if (LL_TIM_IsActiveFlag_UPDATE(cfg->timer)) {
			LL_TIM_ClearFlag_UPDATE(cfg->timer);
			cpt->overflows++;
		}

		if (LL_TIM_IsActiveFlag_CC1(cfg->timer) || LL_TIM_IsActiveFlag_CC2(cfg->timer)) {
			LL_TIM_ClearFlag_CC1(cfg->timer);
			LL_TIM_ClearFlag_CC2(cfg->timer);

			get_hw_timer_capture(dev, in_ch);

			if (cpt->overflows) {
				LOG_ERR("counter overflow during HW_TIMER capture");
				status = -ERANGE;
			}

			if (!cpt->continuous) {
				hw_timer_stm32_disable_capture(dev, in_ch);
			} else {
				cpt->overflows = 0u;
			}

			if (cpt->callback != NULL) {
				cpt->callback(dev, in_ch, cpt->capture_period ? cpt->period : 0u,
					      cpt->capture_pulse ? cpt->pulse : 0u, status,
					      cpt->user_data);
			}
		}
	} else {
		if (LL_TIM_IsActiveFlag_UPDATE(cfg->timer)) {
			LL_TIM_ClearFlag_UPDATE(cfg->timer);
		}

		if (LL_TIM_IsActiveFlag_CC1(cfg->timer) || LL_TIM_IsActiveFlag_CC2(cfg->timer)) {
			LL_TIM_ClearFlag_CC1(cfg->timer);
			LL_TIM_ClearFlag_CC2(cfg->timer);
			cpt->skip_irq--;
		}
	}
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

static int hw_timer_stm32_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					     uint64_t *cycles)
{
	struct hw_timer_stm32_data *data = dev->data;
	const struct hw_timer_stm32_config *cfg = dev->config;

	*cycles = (uint64_t)(data->tim_clk / (cfg->prescaler + 1));

	return 0;
}

/**
 * On timer update.
 */
void hw_timer_stm32_up_isr(const void *arg)
{
	struct device *dev = (struct device *)arg;
	struct hw_timer_stm32_data *data = dev->data;
	const struct hw_timer_stm32_config *cfg = dev->config;

	if (LL_TIM_IsActiveFlag_UPDATE(cfg->timer)) {
		LL_TIM_ClearFlag_UPDATE(cfg->timer);

		if (data->timer_up_callback) {
			(*data->timer_up_callback)(/* dev */);
		}
	}
}

/**
 * Set or remove the update callback.
 */
void hw_timer_stm32_set_update_callback(const struct device *dev, timer_callback_t callback)
{
	struct hw_timer_stm32_data *data = dev->data;
	data->timer_up_callback = callback;
	const struct hw_timer_stm32_config *cfg = dev->config;

	// TODO some timers have 4 IRQs while others have only one
	const int up_irqn = DT_IRQ_BY_IDX(DT_PARENT(DT_DRV_INST(0)), 0, irq);
	const int up_priority = DT_IRQ_BY_IDX(DT_PARENT(DT_DRV_INST(0)), 0, priority);

	LL_TIM_ClearFlag_UPDATE(cfg->timer);

	if (callback) {
		LL_TIM_EnableIT_UPDATE(cfg->timer);
		irq_connect_dynamic(up_irqn, up_priority, hw_timer_stm32_up_isr, dev, 0);

		irq_enable(up_irqn);
	} else {
		irq_disable(up_irqn);
		LL_TIM_DisableIT_UPDATE(cfg->timer);
	}
}

static const struct hw_timer_driver_api hw_timer_stm32_driver_api = {
	.set_cycles = hw_timer_stm32_set_cycles,
	.get_cycles_per_sec = hw_timer_stm32_get_cycles_per_sec,
#ifdef CONFIG_HW_TIMER_CAPTURE
	.configure_capture = hw_timer_stm32_configure_capture,
	.enable_capture = hw_timer_stm32_enable_capture,
	.disable_capture = hw_timer_stm32_disable_capture,
#endif /* CONFIG_HW_TIMER_CAPTURE */
	.set_update_callback = hw_timer_stm32_set_update_callback,
};

static int hw_timer_stm32_init(const struct device *dev)
{
	struct hw_timer_stm32_data *data = dev->data;
	const struct hw_timer_stm32_config *cfg = dev->config;

	int r;
	const struct device *clk;
	LL_TIM_InitTypeDef init;

	/* enable clock and store its speed */
	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	r = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}

	r = get_tim_clk(&cfg->pclken, &data->tim_clk);
	if (r < 0) {
		LOG_ERR("Could not obtain timer clock (%d)", r);
		return r;
	}

	/* configure pinmux */
	r = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (r < 0) {
		LOG_ERR("HW_TIMER pinctrl setup failed (%d)", r);
		return r;
	}

	/* initialize timer */
	LL_TIM_StructInit(&init);

	init.Prescaler = cfg->prescaler;
	init.CounterMode = cfg->countermode;
	init.Autoreload = 0u;
	init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;

	if (LL_TIM_Init(cfg->timer, &init) != SUCCESS) {
		LOG_ERR("Could not initialize timer");
		return -EIO;
	}

#if !defined(CONFIG_SOC_SERIES_STM32L0X) && !defined(CONFIG_SOC_SERIES_STM32L1X)
	/* enable outputs and counter */
	if (IS_TIM_BREAK_INSTANCE(cfg->timer)) {
		LL_TIM_EnableAllOutputs(cfg->timer);
	}
#endif

	LL_TIM_EnableCounter(cfg->timer);

#ifdef CONFIG_HW_TIMER_CAPTURE
	cfg->irq_config_func(dev);
#endif /* CONFIG_HW_TIMER_CAPTURE */

	return 0;
}

#ifdef CONFIG_HW_TIMER_CAPTURE
#define IRQ_CONNECT_AND_ENABLE_BY_NAME(index, name)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), name, irq),                      \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), name, priority),                 \
			    hw_timer_stm32_isr, DEVICE_DT_INST_GET(index), 0);                     \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), name, irq));                      \
	}

#define IRQ_CONNECT_AND_ENABLE_DEFAULT(index)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(index)),                                        \
			    DT_IRQ(DT_INST_PARENT(index), priority), hw_timer_stm32_isr,           \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_IRQN(DT_INST_PARENT(index)));                                        \
	}

#define IRQ_CONFIG_FUNC(index)                                                                     \
	static void hw_timer_stm32_irq_config_func_##index(const struct device *dev)               \
	{                                                                                          \
		COND_CODE_1(DT_IRQ_HAS_NAME(DT_INST_PARENT(index), cc),                            \
			    (IRQ_CONNECT_AND_ENABLE_BY_NAME(index, cc)),                           \
			    (IRQ_CONNECT_AND_ENABLE_DEFAULT(index)));                              \
	}
#define CAPTURE_INIT(index) .irq_config_func = hw_timer_stm32_irq_config_func_##index
#else
#define IRQ_CONFIG_FUNC(index)
#define CAPTURE_INIT(index)
#endif /* CONFIG_HW_TIMER_CAPTURE */

#define DT_INST_CLK(index, inst)                                                                   \
	{                                                                                          \
		.bus = DT_CLOCKS_CELL(DT_INST_PARENT(index), bus),                                 \
		.enr = DT_CLOCKS_CELL(DT_INST_PARENT(index), bits)                                 \
	}

#define HW_TIMER_DEVICE_INIT(index)                                                                \
	static struct hw_timer_stm32_data hw_timer_stm32_data_##index;                             \
	IRQ_CONFIG_FUNC(index)                                                                     \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct hw_timer_stm32_config hw_timer_stm32_config_##index = {                \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(index)),                        \
		.prescaler = DT_PROP(DT_INST_PARENT(index), st_prescaler),                         \
		.countermode = DT_PROP(DT_INST_PARENT(index), st_countermode),                     \
		.pclken = DT_INST_CLK(index, timer),                                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		CAPTURE_INIT(index)                                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &hw_timer_stm32_init, NULL, &hw_timer_stm32_data_##index,     \
			      &hw_timer_stm32_config_##index, POST_KERNEL,                         \
			      CONFIG_HW_TIMER_INIT_PRIORITY, &hw_timer_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HW_TIMER_DEVICE_INIT)

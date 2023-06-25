/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_hw_timer

// #include <errno.h>
// #include <soc.h>
// #include <stm32_ll_rcc.h>
// #include <stm32_ll_tim.h>
// #include "hw_timer.h"
// #include <zephyr/drivers/pinctrl.h>
// #include <zephyr/device.h>
// #include <zephyr/kernel.h>
// #include <zephyr/init.h>
// #include <zephyr/drivers/clock_control/stm32_clock_control.h>
// #include <zephyr/logging/log.h>
// #include <zephyr/irq.h>

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
#include <zephyr/dt-bindings/pwm/stm32_pwm.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(hw_timer_stm32, CONFIG_HW_TIMER_LOG_LEVEL);

/* L0 series MCUs only have 16-bit timers and don't have below macro defined */
#ifndef IS_TIM_32B_COUNTER_INSTANCE
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) (0)
#endif

/** HW_TIMER data. */
struct hw_timer_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
	timer_callback_t timer_up_callback;
};

/** HW_TIMER configuration. */
struct hw_timer_stm32_config {
	TIM_TypeDef *timer;
	uint32_t prescaler;
	uint32_t countermode;
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pcfg;
};

/** Maximum number of timer channels : some stm32 soc have 6 else only 4 */
#if defined(LL_TIM_CHANNEL_CH6)
#define TIMER_HAS_6CH 1
#define TIMER_MAX_CH  6u
#else
#define TIMER_HAS_6CH 0
#define TIMER_MAX_CH  4u
#endif

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_TIM_CHANNEL_CH1, LL_TIM_CHANNEL_CH2, LL_TIM_CHANNEL_CH3, LL_TIM_CHANNEL_CH4,
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
	LL_TIM_OC_SetCompareCH1, LL_TIM_OC_SetCompareCH2, LL_TIM_OC_SetCompareCH3,
	LL_TIM_OC_SetCompareCH4,
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
			oc_init.OCState = LL_TIM_CC_IsEnabledChannel(cfg->timer, ll_channel)
						  ? LL_TIM_OCSTATE_ENABLE
						  : LL_TIM_OCSTATE_DISABLE;
			oc_init.OCPolarity = LL_TIM_OC_GetPolarity(cfg->timer, ll_channel);
		} else {
			oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
			oc_init.OCPolarity = get_polarity(flags);

			/* inherit the polarity of the negative output */
			if (negative_ll_channel) {
				oc_init.OCNState =
					LL_TIM_CC_IsEnabledChannel(cfg->timer, negative_ll_channel)
						? LL_TIM_OCSTATE_ENABLE
						: LL_TIM_OCSTATE_DISABLE;
				oc_init.OCNPolarity =
					LL_TIM_OC_GetPolarity(cfg->timer, negative_ll_channel);
			}
		}
#else  /* LL_TIM_CHANNEL_CH1N */

		oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
		oc_init.OCPolarity = get_polarity(flags);
#endif /* LL_TIM_CHANNEL_CH1N */
		oc_init.CompareValue = pulse_cycles;

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

	return 0;
}

#define IRQ_CONFIG_FUNC(index)
#define CAPTURE_INIT(index)

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
		CAPTURE_INIT(index)};                                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &hw_timer_stm32_init, NULL, &hw_timer_stm32_data_##index,     \
			      &hw_timer_stm32_config_##index, POST_KERNEL,                         \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &hw_timer_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HW_TIMER_DEVICE_INIT)

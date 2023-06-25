/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public HW_TIMER Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HW_TIMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_HW_TIMER_H_

/**
 * @brief HW_TIMER Interface
 * @defgroup hw_timer_interface HW_TIMER Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/toolchain.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define HW_TIMER_POLARITY_NORMAL (0 << 0)
#define HW_TIMER_POLARITY_INVERTED (1 << 0)
#define HW_TIMER_POLARITY_MASK 0x1
#define STM32_HW_TIMER_COMPLEMENTARY (1U << 8)
#define HW_TIMER_STM32_COMPLEMENTARY (1U << 8)
#define STM32_HW_TIMER_COMPLEMENTARY_MASK 0x100

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name HW_TIMER capture configuration flags
 * @anchor HW_TIMER_CAPTURE_FLAGS
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/* Bit 0 is used for HW_TIMER_POLARITY_NORMAL/HW_TIMER_POLARITY_INVERTED */
#define HW_TIMER_CAPTURE_TYPE_SHIFT 1U
#define HW_TIMER_CAPTURE_TYPE_MASK (3U << HW_TIMER_CAPTURE_TYPE_SHIFT)
#define HW_TIMER_CAPTURE_MODE_SHIFT 3U
#define HW_TIMER_CAPTURE_MODE_MASK (1U << HW_TIMER_CAPTURE_MODE_SHIFT)
/** @endcond */

/** HW_TIMER pin capture captures period. */
#define HW_TIMER_CAPTURE_TYPE_PERIOD (1U << HW_TIMER_CAPTURE_TYPE_SHIFT)

/** HW_TIMER pin capture captures pulse width. */
#define HW_TIMER_CAPTURE_TYPE_PULSE (2U << HW_TIMER_CAPTURE_TYPE_SHIFT)

/** HW_TIMER pin capture captures both period and pulse width. */
#define HW_TIMER_CAPTURE_TYPE_BOTH (HW_TIMER_CAPTURE_TYPE_PERIOD | HW_TIMER_CAPTURE_TYPE_PULSE)

/** HW_TIMER pin capture captures a single period/pulse width. */
#define HW_TIMER_CAPTURE_MODE_SINGLE (0U << HW_TIMER_CAPTURE_MODE_SHIFT)

/** HW_TIMER pin capture captures period/pulse width continuously. */
#define HW_TIMER_CAPTURE_MODE_CONTINUOUS (1U << HW_TIMER_CAPTURE_MODE_SHIFT)

/** @} */

/**
 * @brief Provides a type to hold HW_TIMER configuration flags.
 *
 * The lower 8 bits are used for standard flags.
 * The upper 8 bits are reserved for SoC specific flags.
 *
 * @see @ref HW_TIMER_CAPTURE_FLAGS.
 */

typedef uint16_t hw_timer_flags_t;

/**
 * @brief Container for HW_TIMER information specified in devicetree.
 *
 * This type contains a pointer to a HW_TIMER device, channel number (controlled by
 * the HW_TIMER device), the HW_TIMER signal period in nanoseconds and the flags
 * applicable to the channel. Note that not all HW_TIMER drivers support flags. In
 * such case, flags will be set to 0.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_NAME
 * @see HW_TIMER_DT_SPEC_GET_BY_NAME_OR
 * @see HW_TIMER_DT_SPEC_GET_BY_IDX
 * @see HW_TIMER_DT_SPEC_GET_BY_IDX_OR
 * @see HW_TIMER_DT_SPEC_GET
 * @see HW_TIMER_DT_SPEC_GET_OR
 */
struct hw_timer_dt_spec {
	/** HW_TIMER device instance. */
	const struct device *dev;
	/** Channel number. */
	uint32_t channel;
	/** Period in nanoseconds. */
	uint32_t period;
	/** Flags. */
	hw_timer_flags_t flags;
};

/**
 * @brief Static initializer for a struct hw_timer_dt_spec
 *
 * This returns a static initializer for a struct hw_timer_dt_spec given a devicetree
 * node identifier and an index.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *    n: node {
 *        hw_timers = <&hw_timer1 1 1000 HW_TIMER_POLARITY_NORMAL>,
 *               <&hw_timer2 3 2000 HW_TIMER_POLARITY_INVERTED>;
 *        hw_timer-names = "alpha", "beta";
 *    };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    const struct hw_timer_dt_spec spec =
 *        HW_TIMER_DT_SPEC_GET_BY_NAME(DT_NODELABEL(n), alpha);
 *
 *    // Initializes 'spec' to:
 *    // {
 *    //         .dev = DEVICE_DT_GET(DT_NODELABEL(hw_timer1)),
 *    //         .channel = 1,
 *    //         .period = 1000,
 *    //         .flags = HW_TIMER_POLARITY_NORMAL,
 *    // }
 * @endcode
 *
 * The device (dev) must still be checked for readiness, e.g. using
 * device_is_ready(). It is an error to use this macro unless the node exists,
 * has the 'hw_timers' property, and that 'hw_timers' property specifies a HW_TIMER controller,
 * a channel, a period in nanoseconds and optionally flags.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name of a hw_timers element as defined by
 *             the node's hw_timer-names property.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_INST_GET_BY_NAME
 */
#define HW_TIMER_DT_SPEC_GET_BY_NAME(node_id, name)                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_HW_TIMERS_CTLR_BY_NAME(node_id, name)),                    \
		.channel = DT_HW_TIMERS_CHANNEL_BY_NAME(node_id, name),                            \
		.period = DT_HW_TIMERS_PERIOD_BY_NAME(node_id, name),                              \
		.flags = DT_HW_TIMERS_FLAGS_BY_NAME(node_id, name),                                \
	}

/**
 * @brief Static initializer for a struct hw_timer_dt_spec from a DT_DRV_COMPAT
 *        instance.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Lowercase-and-underscores name of a hw_timers element as defined by
 *             the node's hw_timer-names property.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_NAME
 */
#define HW_TIMER_DT_SPEC_INST_GET_BY_NAME(inst, name)                                              \
	HW_TIMER_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Like HW_TIMER_DT_SPEC_GET_BY_NAME(), with a fallback to a default value.
 *
 * If the devicetree node identifier 'node_id' refers to a node with a property
 * 'hw_timers', this expands to <tt>HW_TIMER_DT_SPEC_GET_BY_NAME(node_id, name)</tt>. The
 * @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Devicetree node identifier.
 * @param name Lowercase-and-underscores name of a hw_timers element as defined by
 *             the node's hw_timer-names property
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see HW_TIMER_DT_SPEC_INST_GET_BY_NAME_OR
 */
#define HW_TIMER_DT_SPEC_GET_BY_NAME_OR(node_id, name, default_value)                              \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, hw_timers),                                          \
		    (HW_TIMER_DT_SPEC_GET_BY_NAME(node_id, name)), (default_value))

/**
 * @brief Like HW_TIMER_DT_SPEC_INST_GET_BY_NAME(), with a fallback to a default
 *        value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name Lowercase-and-underscores name of a hw_timers element as defined by
 *             the node's hw_timer-names property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_NAME_OR
 */
#define HW_TIMER_DT_SPEC_INST_GET_BY_NAME_OR(inst, name, default_value)                            \
	HW_TIMER_DT_SPEC_GET_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Static initializer for a struct hw_timer_dt_spec
 *
 * This returns a static initializer for a struct hw_timer_dt_spec given a devicetree
 * node identifier and an index.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *    n: node {
 *        hw_timers = <&hw_timer1 1 1000 HW_TIMER_POLARITY_NORMAL>,
 *               <&hw_timer2 3 2000 HW_TIMER_POLARITY_INVERTED>;
 *    };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    const struct hw_timer_dt_spec spec =
 *        HW_TIMER_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), 1);
 *
 *    // Initializes 'spec' to:
 *    // {
 *    //         .dev = DEVICE_DT_GET(DT_NODELABEL(hw_timer2)),
 *    //         .channel = 3,
 *    //         .period = 2000,
 *    //         .flags = HW_TIMER_POLARITY_INVERTED,
 *    // }
 * @endcode
 *
 * The device (dev) must still be checked for readiness, e.g. using
 * device_is_ready(). It is an error to use this macro unless the node exists,
 * has the 'hw_timers' property, and that 'hw_timers' property specifies a HW_TIMER controller,
 * a channel, a period in nanoseconds and optionally flags.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into 'hw_timers' property.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_INST_GET_BY_IDX
 */
#define HW_TIMER_DT_SPEC_GET_BY_IDX(node_id, idx)                                                  \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_HW_TIMERS_CTLR_BY_IDX(node_id, idx)),                      \
		.channel = DT_HW_TIMERS_CHANNEL_BY_IDX(node_id, idx),                              \
		.period = DT_HW_TIMERS_PERIOD_BY_IDX(node_id, idx),                                \
		.flags = DT_HW_TIMERS_FLAGS_BY_IDX(node_id, idx),                                  \
	}

/**
 * @brief Static initializer for a struct hw_timer_dt_spec from a DT_DRV_COMPAT
 *        instance.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Logical index into 'hw_timers' property.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_IDX
 */
#define HW_TIMER_DT_SPEC_INST_GET_BY_IDX(inst, idx)                                                \
	HW_TIMER_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Like HW_TIMER_DT_SPEC_GET_BY_IDX(), with a fallback to a default value.
 *
 * If the devicetree node identifier 'node_id' refers to a node with a property
 * 'hw_timers', this expands to <tt>HW_TIMER_DT_SPEC_GET_BY_IDX(node_id, idx)</tt>. The
 * @p default_value parameter is not expanded in this case. Otherwise, this
 * expands to @p default_value.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Logical index into 'hw_timers' property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see HW_TIMER_DT_SPEC_INST_GET_BY_IDX_OR
 */
#define HW_TIMER_DT_SPEC_GET_BY_IDX_OR(node_id, idx, default_value)                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, hw_timers),                                          \
		    (HW_TIMER_DT_SPEC_GET_BY_IDX(node_id, idx)), (default_value))

/**
 * @brief Like HW_TIMER_DT_SPEC_INST_GET_BY_IDX(), with a fallback to a default
 *        value.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Logical index into 'hw_timers' property.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property,
 *         or @p default_value if the node or property do not exist.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_IDX_OR
 */
#define HW_TIMER_DT_SPEC_INST_GET_BY_IDX_OR(inst, idx, default_value)                              \
	HW_TIMER_DT_SPEC_GET_BY_IDX_OR(DT_DRV_INST(inst), idx, default_value)

/**
 * @brief Equivalent to <tt>HW_TIMER_DT_SPEC_GET_BY_IDX(node_id, 0)</tt>.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_IDX
 * @see HW_TIMER_DT_SPEC_INST_GET
 */
#define HW_TIMER_DT_SPEC_GET(node_id) HW_TIMER_DT_SPEC_GET_BY_IDX(node_id, 0)

/**
 * @brief Equivalent to <tt>HW_TIMER_DT_SPEC_INST_GET_BY_IDX(inst, 0)</tt>.
 *
 * @param inst DT_DRV_COMPAT instance number
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_INST_GET_BY_IDX
 * @see HW_TIMER_DT_SPEC_GET
 */
#define HW_TIMER_DT_SPEC_INST_GET(inst) HW_TIMER_DT_SPEC_GET(DT_DRV_INST(inst))

/**
 * @brief Equivalent to
 *        <tt>HW_TIMER_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value)</tt>.
 *
 * @param node_id Devicetree node identifier.
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_GET_BY_IDX_OR
 * @see HW_TIMER_DT_SPEC_INST_GET_OR
 */
#define HW_TIMER_DT_SPEC_GET_OR(node_id, default_value)                                            \
	HW_TIMER_DT_SPEC_GET_BY_IDX_OR(node_id, 0, default_value)

/**
 * @brief Equivalent to
 *        <tt>HW_TIMER_DT_SPEC_INST_GET_BY_IDX_OR(inst, 0, default_value)</tt>.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param default_value Fallback value to expand to.
 *
 * @return Static initializer for a struct hw_timer_dt_spec for the property.
 *
 * @see HW_TIMER_DT_SPEC_INST_GET_BY_IDX_OR
 * @see HW_TIMER_DT_SPEC_GET_OR
 */
#define HW_TIMER_DT_SPEC_INST_GET_OR(inst, default_value)                                          \
	HW_TIMER_DT_SPEC_GET_OR(DT_DRV_INST(inst), default_value)

/**
 * @brief HW_TIMER capture callback handler function signature
 *
 * @note The callback handler will be called in interrupt context.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected to enable HW_TIMER capture
 * support.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.

 * @param period_cycles Captured HW_TIMER period width (in clock cycles). HW
 *                      specific.
 * @param pulse_cycles Captured HW_TIMER pulse width (in clock cycles). HW specific.
 * @param status Status for the HW_TIMER capture (0 if no error, negative errno
 *               otherwise. See hw_timer_capture_cycles() return value
 *               descriptions for details).
 * @param user_data User data passed to hw_timer_configure_capture()
 */
typedef void (*hw_timer_capture_callback_handler_t)(const struct device *dev, uint32_t channel,
						    uint32_t period_cycles, uint32_t pulse_cycles,
						    int status, void *user_data);

/** @cond INTERNAL_HIDDEN */
/**
 * @brief HW_TIMER driver API call to configure HW_TIMER pin period and pulse width.
 * @see hw_timer_set_cycles() for argument description.
 */
typedef int (*hw_timer_set_cycles_t)(const struct device *dev, uint32_t channel,
				     uint32_t period_cycles, uint32_t pulse_cycles,
				     hw_timer_flags_t flags);

/**
 * @brief HW_TIMER driver API call to obtain the HW_TIMER cycles per second (frequency).
 * @see hw_timer_get_cycles_per_sec() for argument description
 */
typedef int (*hw_timer_get_cycles_per_sec_t)(const struct device *dev, uint32_t channel,
					     uint64_t *cycles);

#ifdef CONFIG_HW_TIMER_CAPTURE
/**
 * @brief HW_TIMER driver API call to configure HW_TIMER capture.
 * @see hw_timer_configure_capture() for argument description.
 */
typedef int (*hw_timer_configure_capture_t)(const struct device *dev, uint32_t channel,
					    hw_timer_flags_t flags,
					    hw_timer_capture_callback_handler_t cb,
					    void *user_data);

/**
 * @brief HW_TIMER driver API call to enable HW_TIMER capture.
 * @see hw_timer_enable_capture() for argument description.
 */
typedef int (*hw_timer_enable_capture_t)(const struct device *dev, uint32_t channel);

/**
 * @brief HW_TIMER driver API call to disable HW_TIMER capture.
 * @see hw_timer_disable_capture() for argument description
 */
typedef int (*hw_timer_disable_capture_t)(const struct device *dev, uint32_t channel);
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * User callback on timer update
 */
typedef void (*timer_callback_t)(/* const struct device *dev */);

/**
 *
 */
typedef void (*hw_timer_set_update_callback_t)(const struct device *dev, timer_callback_t callback);

/** @brief HW_TIMER driver API definition. */
__subsystem struct hw_timer_driver_api {
	hw_timer_set_cycles_t set_cycles;
	hw_timer_get_cycles_per_sec_t get_cycles_per_sec;
#ifdef CONFIG_HW_TIMER_CAPTURE
	hw_timer_configure_capture_t configure_capture;
	hw_timer_enable_capture_t enable_capture;
	hw_timer_disable_capture_t disable_capture;
#endif /* CONFIG_HW_TIMER_CAPTURE */
	hw_timer_set_update_callback_t set_update_callback;
};
/** @endcond */

/**
 * @brief Set the period and pulse width for a single HW_TIMER output.
 *
 * The HW_TIMER period and pulse width will synchronously be set to the new values
 * without glitches in the HW_TIMER signal, but the call will not block for the
 * change to take effect.
 *
 * @note Not all HW_TIMER controllers support synchronous, glitch-free updates of the
 * HW_TIMER period and pulse width. Depending on the hardware, changing the HW_TIMER
 * period and/or pulse width may cause a glitch in the generated HW_TIMER signal.
 *
 * @note Some multi-channel HW_TIMER controllers share the HW_TIMER period across all
 * channels. Depending on the hardware, changing the HW_TIMER period for one channel
 * may affect the HW_TIMER period for the other channels of the same HW_TIMER controller.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param period Period (in clock cycles) set to the HW_TIMER. HW specific.
 * @param pulse Pulse width (in clock cycles) set to the HW_TIMER. HW specific.
 * @param flags Flags for pin configuration.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If pulse > period.
 * @retval -errno Negative errno code on failure.
 */
// clang-format off
__syscall int hw_timer_set_cycles(const struct device *dev, uint32_t channel,
				uint32_t period, uint32_t pulse,
				hw_timer_flags_t flags);

static inline int z_impl_hw_timer_set_cycles(const struct device *dev,
					uint32_t channel, uint32_t period,
					uint32_t pulse, hw_timer_flags_t flags)
// clang-format on
{
	const struct hw_timer_driver_api *api = (const struct hw_timer_driver_api *)dev->api;

	if (pulse > period) {
		return -EINVAL;
	}

	return api->set_cycles(dev, channel, period, pulse, flags);
}

/**
 * @brief Get the clock rate (cycles per second) for a single HW_TIMER output.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param[out] cycles Pointer to the memory to store clock rate (cycles per
 *                    sec). HW specific.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
__syscall int hw_timer_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					  uint64_t *cycles);

static inline int z_impl_hw_timer_get_cycles_per_sec(const struct device *dev, uint32_t channel,
						     uint64_t *cycles)
{
	const struct hw_timer_driver_api *api = (const struct hw_timer_driver_api *)dev->api;

	return api->get_cycles_per_sec(dev, channel, cycles);
}

/**
 * @brief Set the period and pulse width in nanoseconds for a single HW_TIMER output.
 *
 * @note Utility macros such as HW_TIMER_MSEC() can be used to convert from other
 * scales or units to nanoseconds, the units used by this function.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param period Period (in nanoseconds) set to the HW_TIMER.
 * @param pulse Pulse width (in nanoseconds) set to the HW_TIMER.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If requested period or pulse cycles are not supported.
 * @retval -errno Other negative errno code on failure.
 */
static inline int hw_timer_set(const struct device *dev, uint32_t channel, uint32_t period,
			       uint32_t pulse, hw_timer_flags_t flags)
{
	int err;
	uint64_t pulse_cycles;
	uint64_t period_cycles;
	uint64_t cycles_per_sec;

	err = hw_timer_get_cycles_per_sec(dev, channel, &cycles_per_sec);
	if (err < 0) {
		return err;
	}

	period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
	if (period_cycles > UINT32_MAX) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC;
	if (pulse_cycles > UINT32_MAX) {
		return -ENOTSUP;
	}

	return hw_timer_set_cycles(dev, channel, (uint32_t)period_cycles, (uint32_t)pulse_cycles,
				   flags);
}

/**
 * @brief Set the period and pulse width in nanoseconds from a struct
 *        hw_timer_dt_spec (with custom period).
 *
 * This is equivalent to:
 *
 *     hw_timer_set(spec->dev, spec->channel, period, pulse, spec->flags)
 *
 * The period specified in @p spec is ignored. This API call can be used when
 * the period specified in Devicetree needs to be changed at runtime.
 *
 * @param[in] spec HW_TIMER specification from devicetree.
 * @param period Period (in nanoseconds) set to the HW_TIMER.
 * @param pulse Pulse width (in nanoseconds) set to the HW_TIMER.
 *
 * @return A value from hw_timer_set().
 *
 * @see hw_timer_set_pulse_dt()
 */
static inline int hw_timer_set_dt(const struct hw_timer_dt_spec *spec, uint32_t period,
				  uint32_t pulse)
{
	return hw_timer_set(spec->dev, spec->channel, period, pulse, spec->flags);
}

/**
 * @brief Set the period and pulse width in nanoseconds from a struct
 *        hw_timer_dt_spec.
 *
 * This is equivalent to:
 *
 *     hw_timer_set(spec->dev, spec->channel, spec->period, pulse, spec->flags)
 *
 * @param[in] spec HW_TIMER specification from devicetree.
 * @param pulse Pulse width (in nanoseconds) set to the HW_TIMER.
 *
 * @return A value from hw_timer_set().
 *
 * @see hw_timer_set_pulse_dt()
 */
static inline int hw_timer_set_pulse_dt(const struct hw_timer_dt_spec *spec, uint32_t pulse)
{
	return hw_timer_set(spec->dev, spec->channel, spec->period, pulse, spec->flags);
}

/**
 * @brief Convert from HW_TIMER cycles to microseconds.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param cycles Cycles to be converted.
 * @param[out] usec Pointer to the memory to store calculated usec.
 *
 * @retval 0 If successful.
 * @retval -ERANGE If result is too large.
 * @retval -errno Other negative errno code on failure.
 */
static inline int hw_timer_cycles_to_usec(const struct device *dev, uint32_t channel,
					  uint32_t cycles, uint64_t *usec)
{
	int err;
	uint64_t temp;
	uint64_t cycles_per_sec;

	err = hw_timer_get_cycles_per_sec(dev, channel, &cycles_per_sec);
	if (err < 0) {
		return err;
	}

	if (u64_mul_overflow(cycles, (uint64_t)USEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*usec = temp / cycles_per_sec;

	return 0;
}

/**
 * @brief Convert from HW_TIMER cycles to nanoseconds.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param cycles Cycles to be converted.
 * @param[out] nsec Pointer to the memory to store the calculated nsec.
 *
 * @retval 0 If successful.
 * @retval -ERANGE If result is too large.
 * @retval -errno Other negative errno code on failure.
 */
static inline int hw_timer_cycles_to_nsec(const struct device *dev, uint32_t channel,
					  uint32_t cycles, uint64_t *nsec)
{
	int err;
	uint64_t temp;
	uint64_t cycles_per_sec;

	err = hw_timer_get_cycles_per_sec(dev, channel, &cycles_per_sec);
	if (err < 0) {
		return err;
	}

	if (u64_mul_overflow(cycles, (uint64_t)NSEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*nsec = temp / cycles_per_sec;

	return 0;
}

#if defined(CONFIG_HW_TIMER_CAPTURE) || defined(__DOXYGEN__)
/**
 * @brief Configure HW_TIMER period/pulse width capture for a single HW_TIMER input.
 *
 * After configuring HW_TIMER capture using this function, the capture can be
 * enabled/disabled using hw_timer_enable_capture() and
 * hw_timer_disable_capture().
 *
 * @note This API function cannot be invoked from user space due to the use of a
 * function callback. In user space, one of the simpler API functions
 * (hw_timer_capture_cycles(), hw_timer_capture_usec(), or
 * hw_timer_capture_nsec()) can be used instead.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param flags HW_TIMER capture flags
 * @param[in] cb Application callback handler function to be called upon capture
 * @param[in] user_data User data to pass to the application callback handler
 *                      function
 *
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if HW_TIMER capture is not supported or the given flags are not
 *                  supported
 * @retval -EIO if IO error occurred while configuring
 * @retval -EBUSY if HW_TIMER capture is already in progress
 */
static inline int hw_timer_configure_capture(const struct device *dev, uint32_t channel,
					     hw_timer_flags_t flags,
					     hw_timer_capture_callback_handler_t cb,
					     void *user_data)
{
	const struct hw_timer_driver_api *api = (const struct hw_timer_driver_api *)dev->api;

	if (api->configure_capture == NULL) {
		return -ENOSYS;
	}

	return api->configure_capture(dev, channel, flags, cb, user_data);
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * @brief Enable HW_TIMER period/pulse width capture for a single HW_TIMER input.
 *
 * The HW_TIMER pin must be configured using hw_timer_configure_capture() prior to
 * calling this function.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if HW_TIMER capture is not supported
 * @retval -EIO if IO error occurred while enabling HW_TIMER capture
 * @retval -EBUSY if HW_TIMER capture is already in progress
 */
__syscall int hw_timer_enable_capture(const struct device *dev, uint32_t channel);

#ifdef CONFIG_HW_TIMER_CAPTURE
static inline int z_impl_hw_timer_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct hw_timer_driver_api *api = (const struct hw_timer_driver_api *)dev->api;

	if (api->enable_capture == NULL) {
		return -ENOSYS;
	}

	return api->enable_capture(dev, channel);
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * @brief Disable HW_TIMER period/pulse width capture for a single HW_TIMER input.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if HW_TIMER capture is not supported
 * @retval -EIO if IO error occurred while disabling HW_TIMER capture
 */
__syscall int hw_timer_disable_capture(const struct device *dev, uint32_t channel);

#ifdef CONFIG_HW_TIMER_CAPTURE
static inline int z_impl_hw_timer_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct hw_timer_driver_api *api = (const struct hw_timer_driver_api *)dev->api;

	if (api->disable_capture == NULL) {
		return -ENOSYS;
	}

	return api->disable_capture(dev, channel);
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * @brief Capture a single HW_TIMER period/pulse width in clock cycles for a single
 *        HW_TIMER input.
 *
 * This API function wraps calls to hw_timer_configure_capture(),
 * hw_timer_enable_capture(), and hw_timer_disable_capture() and passes
 * the capture result to the caller. The function is blocking until either the
 * HW_TIMER capture is completed or a timeout occurs.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param flags HW_TIMER capture flags.
 * @param[out] period Pointer to the memory to store the captured HW_TIMER period
 *                    width (in clock cycles). HW specific.
 * @param[out] pulse Pointer to the memory to store the captured HW_TIMER pulse width
 *                   (in clock cycles). HW specific.
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY HW_TIMER capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
__syscall int hw_timer_capture_cycles(const struct device *dev, uint32_t channel,
				      hw_timer_flags_t flags, uint32_t *period, uint32_t *pulse,
				      k_timeout_t timeout);

/**
 *
 */
__syscall void hw_timer_set_update_callback(const struct device *dev, timer_callback_t callback);
static inline void z_impl_hw_timer_set_update_callback(const struct device *dev,
						       timer_callback_t callback)
{
	struct hw_timer_driver_api *api = (struct hw_timer_driver_api *)dev->api;
	api->set_update_callback(dev, callback);
}

/**
 * @brief Capture a single HW_TIMER period/pulse width in microseconds for a single
 *        HW_TIMER input.
 *
 * This API function wraps calls to hw_timer_capture_cycles() and
 * hw_timer_cycles_to_usec() and passes the capture result to the caller. The
 * function is blocking until either the HW_TIMER capture is completed or a timeout
 * occurs.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param flags HW_TIMER capture flags.
 * @param[out] period Pointer to the memory to store the captured HW_TIMER period
 *                    width (in usec).
 * @param[out] pulse Pointer to the memory to store the captured HW_TIMER pulse width
 *                   (in usec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY HW_TIMER capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 * @retval -errno Other negative errno code on failure.
 */
static inline int hw_timer_capture_usec(const struct device *dev, uint32_t channel,
					hw_timer_flags_t flags, uint64_t *period, uint64_t *pulse,
					k_timeout_t timeout)
{
	int err;
	uint32_t pulse_cycles;
	uint32_t period_cycles;

	err = hw_timer_capture_cycles(dev, channel, flags, &period_cycles, &pulse_cycles, timeout);
	if (err < 0) {
		return err;
	}

	err = hw_timer_cycles_to_usec(dev, channel, period_cycles, period);
	if (err < 0) {
		return err;
	}

	err = hw_timer_cycles_to_usec(dev, channel, pulse_cycles, pulse);
	if (err < 0) {
		return err;
	}

	return 0;
}

/**
 * @brief Capture a single HW_TIMER period/pulse width in nanoseconds for a single
 *        HW_TIMER input.
 *
 * This API function wraps calls to hw_timer_capture_cycles() and
 * hw_timer_cycles_to_nsec() and passes the capture result to the caller. The
 * function is blocking until either the HW_TIMER capture is completed or a timeout
 * occurs.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param[in] dev HW_TIMER device instance.
 * @param channel HW_TIMER channel.
 * @param flags HW_TIMER capture flags.
 * @param[out] period Pointer to the memory to store the captured HW_TIMER period
 *                    width (in nsec).
 * @param[out] pulse Pointer to the memory to store the captured HW_TIMER pulse width
 *                   (in nsec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY HW_TIMER capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 * @retval -errno Other negative errno code on failure.
 */
static inline int hw_timer_capture_nsec(const struct device *dev, uint32_t channel,
					hw_timer_flags_t flags, uint64_t *period, uint64_t *pulse,
					k_timeout_t timeout)
{
	int err;
	uint32_t pulse_cycles;
	uint32_t period_cycles;

	err = hw_timer_capture_cycles(dev, channel, flags, &period_cycles, &pulse_cycles, timeout);
	if (err < 0) {
		return err;
	}

	err = hw_timer_cycles_to_nsec(dev, channel, period_cycles, period);
	if (err < 0) {
		return err;
	}

	err = hw_timer_cycles_to_nsec(dev, channel, pulse_cycles, pulse);
	if (err < 0) {
		return err;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/hw_timer.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HW_TIMER_H_ */

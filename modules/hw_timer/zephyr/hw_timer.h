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
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/math_extras.h>
#include <device.h>

#define HW_TIMER_POLARITY_NORMAL (0 << 0)

/** HW_TIMER pin inverted polarity (active-low pulse). */
#define HW_TIMER_POLARITY_INVERTED (1 << 0)

/** @cond INTERNAL_HIDDEN */
#define HW_TIMER_POLARITY_MASK 0x1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name HW_TIMER capture configuration flags
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
 */
typedef uint8_t hw_timer_flags_t;

/**
 * @typedef hw_timer_pin_set_t
 * @brief Callback API upon setting the pin
 * See @a hw_timer_pin_set_cycles() for argument description
 */
typedef int (*hw_timer_pin_set_t)(const struct device *dev, uint32_t hw_timer,
				  uint32_t period_cycles, uint32_t pulse_cycles,
				  hw_timer_flags_t flags);

/**
 * @typedef hw_timer_capture_callback_handler_t
 * @brief HW_TIMER capture callback handler function signature
 *
 * @note The callback handler will be called in interrupt context.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected to enable HW_TIMER capture
 * support.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.

 * @param period_cycles Captured HW_TIMER period width (in clock cycles). HW
 *                      specific.
 * @param pulse_cycles Captured HW_TIMER pulse width (in clock cycles). HW specific.
 * @param status Status for the HW_TIMER capture (0 if no error, negative errno
 *               otherwise. See @a hw_timer_pin_capture_cycles() return value
 *               descriptions for details).
 * @param user_data User data passed to @a hw_timer_pin_configure_capture()
 */
typedef void (*hw_timer_capture_callback_handler_t)(const struct device *dev, uint32_t hw_timer,
						    uint32_t period_cycles, uint32_t pulse_cycles,
						    int status, void *user_data);

/**
 * @typedef hw_timer_pin_configure_capture_t
 * @brief Callback API upon configuring HW_TIMER pin capture
 * See @a hw_timer_pin_configure_capture() for argument description
 */
typedef int (*hw_timer_pin_configure_capture_t)(const struct device *dev, uint32_t hw_timer,
						hw_timer_flags_t flags,
						hw_timer_capture_callback_handler_t cb,
						void *user_data);
/**
 * @typedef hw_timer_pin_enable_capture_t
 * @brief Callback API upon enabling HW_TIMER pin capture
 * See @a hw_timer_pin_enable_capture() for argument description
 */
typedef int (*hw_timer_pin_enable_capture_t)(const struct device *dev, uint32_t hw_timer);

/**
 * @typedef hw_timer_pin_disable_capture_t
 * @brief Callback API upon disabling HW_TIMER pin capture
 * See @a hw_timer_pin_disable_capture() for argument description
 */
typedef int (*hw_timer_pin_disable_capture_t)(const struct device *dev, uint32_t hw_timer);

/**
 * @typedef hw_timer_get_cycles_per_sec_t
 * @brief Callback API upon getting cycles per second
 * See @a hw_timer_get_cycles_per_sec() for argument description
 */
typedef int (*hw_timer_get_cycles_per_sec_t)(const struct device *dev, uint32_t hw_timer,
					     uint64_t *cycles);

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
	hw_timer_pin_set_t pin_set;
#ifdef CONFIG_HW_TIMER_CAPTURE
	hw_timer_pin_configure_capture_t pin_configure_capture;
	hw_timer_pin_enable_capture_t pin_enable_capture;
	hw_timer_pin_disable_capture_t pin_disable_capture;
#endif /* CONFIG_HW_TIMER_CAPTURE */
	hw_timer_get_cycles_per_sec_t get_cycles_per_sec;
	hw_timer_set_update_callback_t set_update_callback;
};

/**
 * @brief Set the period and pulse width for a single HW_TIMER output.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param period Period (in clock cycle) set to the HW_TIMER. HW specific.
 * @param pulse Pulse width (in clock cycle) set to the HW_TIMER. HW specific.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int hw_timer_pin_set_cycles(const struct device *dev, uint32_t hw_timer, uint32_t period,
				      uint32_t pulse, hw_timer_flags_t flags);

static inline int z_impl_hw_timer_pin_set_cycles(const struct device *dev, uint32_t hw_timer,
						 uint32_t period, uint32_t pulse,
						 hw_timer_flags_t flags)
{
	struct hw_timer_driver_api *api;

	api = (struct hw_timer_driver_api *)dev->api;
	return api->pin_set(dev, hw_timer, period, pulse, flags);
}

/**
 * @brief Configure HW_TIMER period/pulse width capture for a single HW_TIMER input.
 *
 * After configuring HW_TIMER capture using this function, the capture can be
 * enabled/disabled using @a hw_timer_pin_enable_capture() and @a
 * hw_timer_pin_disable_capture().
 *
 * @note This API function cannot be invoked from user space due to the use of a
 * function callback. In user space, one of the simpler API functions (@a
 * hw_timer_pin_capture_cycles(), @a hw_timer_pin_capture_usec(), or @a
 * hw_timer_pin_capture_nsec()) can be used instead.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param flags HW_TIMER capture flags
 * @param cb Application callback handler function to be called upon capture
 * @param user_data User data to pass to the application callback handler
 *                  function
 *
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if HW_TIMER capture is not supported or the given flags are not
 *                  supported
 * @retval -EIO if IO error occurred while configuring
 * @retval -EBUSY if HW_TIMER capture is already in progress
 */
#ifdef CONFIG_HW_TIMER_CAPTURE
static inline int hw_timer_pin_configure_capture(const struct device *dev, uint32_t hw_timer,
						 hw_timer_flags_t flags,
						 hw_timer_capture_callback_handler_t cb,
						 void *user_data)
{
	const struct hw_timer_driver_api *api = (struct hw_timer_driver_api *)dev->api;

	if (api->pin_configure_capture == NULL) {
		return -ENOSYS;
	}

	return api->pin_configure_capture(dev, hw_timer, flags, cb, user_data);
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * @brief Enable HW_TIMER period/pulse width capture for a single HW_TIMER input.
 *
 * The HW_TIMER pin must be configured using @a hw_timer_pin_configure_capture() prior to
 * calling this function.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if HW_TIMER capture is not supported
 * @retval -EIO if IO error occurred while enabling HW_TIMER capture
 * @retval -EBUSY if HW_TIMER capture is already in progress
 */
__syscall int hw_timer_pin_enable_capture(const struct device *dev, uint32_t hw_timer);

#ifdef CONFIG_HW_TIMER_CAPTURE
static inline int z_impl_hw_timer_pin_enable_capture(const struct device *dev, uint32_t hw_timer)
{
	const struct hw_timer_driver_api *api = (struct hw_timer_driver_api *)dev->api;

	if (api->pin_enable_capture == NULL) {
		return -ENOSYS;
	}

	return api->pin_enable_capture(dev, hw_timer);
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * @brief Disable HW_TIMER period/pulse width capture for a single HW_TIMER input.
 *
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if invalid function parameters were given
 * @retval -ENOSYS if HW_TIMER capture is not supported
 * @retval -EIO if IO error occurred while disabling HW_TIMER capture
 */
__syscall int hw_timer_pin_disable_capture(const struct device *dev, uint32_t hw_timer);

#ifdef CONFIG_HW_TIMER_CAPTURE
static inline int z_impl_hw_timer_pin_disable_capture(const struct device *dev, uint32_t hw_timer)
{
	const struct hw_timer_driver_api *api = (struct hw_timer_driver_api *)dev->api;

	if (api->pin_disable_capture == NULL) {
		return -ENOSYS;
	}

	return api->pin_disable_capture(dev, hw_timer);
}
#endif /* CONFIG_HW_TIMER_CAPTURE */

/**
 * @brief Capture a single HW_TIMER period/pulse width in clock cycles for a single
 *        HW_TIMER input.
 *
 * This API function wraps calls to @a hw_timer_pin_configure_capture(), @a
 * hw_timer_pin_enable_capture(), and @a hw_timer_pin_disable_capture() and passes the
 * capture result to the caller. The function is blocking until either the HW_TIMER
 * capture is completed or a timeout occurs.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param flags HW_TIMER capture flags.
 * @param period Pointer to the memory to store the captured HW_TIMER period width
 *               (in clock cycles). HW specific.
 * @param pulse Pointer to the memory to store the captured HW_TIMER pulse width (in
 *              clock cycles). HW specific.
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY HW_TIMER capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
__syscall int hw_timer_pin_capture_cycles(const struct device *dev, uint32_t hw_timer,
					  hw_timer_flags_t flags, uint32_t *period, uint32_t *pulse,
					  k_timeout_t timeout);

/**
 * @brief Get the clock rate (cycles per second) for a single HW_TIMER output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param cycles Pointer to the memory to store clock rate (cycles per sec).
 *		 HW specific.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int hw_timer_get_cycles_per_sec(const struct device *dev, uint32_t hw_timer,
					  uint64_t *cycles);

static inline int z_impl_hw_timer_get_cycles_per_sec(const struct device *dev, uint32_t hw_timer,
						     uint64_t *cycles)
{
	struct hw_timer_driver_api *api;

	api = (struct hw_timer_driver_api *)dev->api;
	return api->get_cycles_per_sec(dev, hw_timer, cycles);
}

/**
 * 
 */
__syscall void hw_timer_set_update_callback(const struct device *dev, timer_callback_t callback);
static inline void z_impl_hw_timer_set_update_callback(const struct device *dev,
						       timer_callback_t callback)
{
	struct hw_timer_driver_api *api;
	api = (struct hw_timer_driver_api *)dev->api;
	api->set_update_callback(dev, callback);
}

/**
 * @brief Set the period and pulse width for a single HW_TIMER output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param period Period (in microseconds) set to the HW_TIMER.
 * @param pulse Pulse width (in microseconds) set to the HW_TIMER.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int hw_timer_pin_set_usec(const struct device *dev, uint32_t hw_timer,
					uint32_t period, uint32_t pulse, hw_timer_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (hw_timer_get_cycles_per_sec(dev, hw_timer, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / USEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / USEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return hw_timer_pin_set_cycles(dev, hw_timer, (uint32_t)period_cycles,
				       (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Set the period and pulse width for a single HW_TIMER output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param period Period (in nanoseconds) set to the HW_TIMER.
 * @param pulse Pulse width (in nanoseconds) set to the HW_TIMER.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int hw_timer_pin_set_nsec(const struct device *dev, uint32_t hw_timer,
					uint32_t period, uint32_t pulse, hw_timer_flags_t flags)
{
	uint64_t period_cycles, pulse_cycles, cycles_per_sec;

	if (hw_timer_get_cycles_per_sec(dev, hw_timer, &cycles_per_sec) != 0) {
		return -EIO;
	}

	period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
	if (period_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC;
	if (pulse_cycles >= ((uint64_t)1 << 32)) {
		return -ENOTSUP;
	}

	return hw_timer_pin_set_cycles(dev, hw_timer, (uint32_t)period_cycles,
				       (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Convert from HW_TIMER cycles to microseconds.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param cycles Cycles to be converted.
 * @param usec Pointer to the memory to store calculated usec.
 *
 * @retval 0 If successful.
 * @retval -EIO If cycles per second cannot be determined.
 * @retval -ERANGE If result is too large.
 */
static inline int hw_timer_pin_cycles_to_usec(const struct device *dev, uint32_t hw_timer,
					      uint32_t cycles, uint64_t *usec)
{
	uint64_t cycles_per_sec;
	uint64_t temp;

	if (hw_timer_get_cycles_per_sec(dev, hw_timer, &cycles_per_sec) != 0) {
		return -EIO;
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
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param cycles Cycles to be converted.
 * @param nsec Pointer to the memory to store the calculated nsec.
 *
 * @retval 0 If successful.
 * @retval -EIO If cycles per second cannot be determined.
 * @retval -ERANGE If result is too large.
 */
static inline int hw_timer_pin_cycles_to_nsec(const struct device *dev, uint32_t hw_timer,
					      uint32_t cycles, uint64_t *nsec)
{
	uint64_t cycles_per_sec;
	uint64_t temp;

	if (hw_timer_get_cycles_per_sec(dev, hw_timer, &cycles_per_sec) != 0) {
		return -EIO;
	}

	if (u64_mul_overflow(cycles, (uint64_t)NSEC_PER_SEC, &temp)) {
		return -ERANGE;
	}

	*nsec = temp / cycles_per_sec;

	return 0;
}

/**
 * @brief Capture a single HW_TIMER period/pulse width in microseconds for a single
 *        HW_TIMER input.
 *
 * This API function wraps calls to @a hw_timer_pin_capture_cycles() and @a
 * hw_timer_pin_cycles_to_usec() and passes the capture result to the caller. The
 * function is blocking until either the HW_TIMER capture is completed or a timeout
 * occurs.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param flags HW_TIMER capture flags.
 * @param period Pointer to the memory to store the captured HW_TIMER period width
 *               (in usec).
 * @param pulse Pointer to the memory to store the captured HW_TIMER pulse width (in
 *              usec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY HW_TIMER capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
static inline int hw_timer_pin_capture_usec(const struct device *dev, uint32_t hw_timer,
					    hw_timer_flags_t flags, uint64_t *period,
					    uint64_t *pulse, k_timeout_t timeout)
{
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	int err;

	err = hw_timer_pin_capture_cycles(dev, hw_timer, flags, &period_cycles, &pulse_cycles,
					  timeout);
	if (err) {
		return err;
	}

	err = hw_timer_pin_cycles_to_usec(dev, hw_timer, period_cycles, period);
	if (err) {
		return err;
	}

	err = hw_timer_pin_cycles_to_usec(dev, hw_timer, pulse_cycles, pulse);
	if (err) {
		return err;
	}

	return 0;
}

/**
 * @brief Capture a single HW_TIMER period/pulse width in nanoseconds for a single
 *        HW_TIMER input.
 *
 * This API function wraps calls to @a hw_timer_pin_capture_cycles() and @a
 * hw_timer_pin_cycles_to_nsec() and passes the capture result to the caller. The
 * function is blocking until either the HW_TIMER capture is completed or a timeout
 * occurs.
 *
 * @note @kconfig{CONFIG_HW_TIMER_CAPTURE} must be selected for this function to be
 * available.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param hw_timer HW_TIMER pin.
 * @param flags HW_TIMER capture flags.
 * @param period Pointer to the memory to store the captured HW_TIMER period width
 *               (in nsec).
 * @param pulse Pointer to the memory to store the captured HW_TIMER pulse width (in
 *              nsec).
 * @param timeout Waiting period for the capture to complete.
 *
 * @retval 0 If successful.
 * @retval -EBUSY HW_TIMER capture already in progress.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EIO IO error while capturing.
 * @retval -ERANGE If result is too large.
 */
static inline int hw_timer_pin_capture_nsec(const struct device *dev, uint32_t hw_timer,
					    hw_timer_flags_t flags, uint64_t *period,
					    uint64_t *pulse, k_timeout_t timeout)
{
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	int err;

	err = hw_timer_pin_capture_cycles(dev, hw_timer, flags, &period_cycles, &pulse_cycles,
					  timeout);
	if (err) {
		return err;
	}

	err = hw_timer_pin_cycles_to_nsec(dev, hw_timer, period_cycles, period);
	if (err) {
		return err;
	}

	err = hw_timer_pin_cycles_to_nsec(dev, hw_timer, pulse_cycles, pulse);
	if (err) {
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

#include "grbl.h"

void spindle_init () {}

uint8_t spindle_get_state () { return (SPINDLE_STATE_DISABLE); }

void spindle_stop () {}

#ifdef VARIABLE_SPINDLE
void spindle_set_speed (uint8_t pwm_value) {}

uint8_t spindle_compute_pwm_value (float rpm) { return (SPINDLE_PWM_OFF_VALUE); }

#endif

#ifdef VARIABLE_SPINDLE
void spindle_set_state (uint8_t state, float rpm)
#else
void _spindle_set_state (uint8_t state)
#endif
{
}

#ifdef VARIABLE_SPINDLE
void spindle_sync (uint8_t state, float rpm) {}
#else
void _spindle_sync (uint8_t state) {}
#endif

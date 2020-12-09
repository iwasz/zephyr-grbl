#include "grbl.h"

void coolant_init () {}
uint8_t coolant_get_state () { return (COOLANT_STATE_DISABLE); }
void coolant_stop () {}
void coolant_set_state (uint8_t mode) {}
void coolant_sync (uint8_t mode) {}

#ifndef SB_RECOVERED_APPLICATION_STATE_H
#define SB_RECOVERED_APPLICATION_STATE_H

#include <stdint.h>
#include "nested_field.h"

/* Neutral representation of the globals directly written by the 0x2af9
 * branch. Their application semantics are intentionally not assigned. */
typedef struct SBRecoveredApplicationState {
    uint16_t global_01de;
    uint8_t global_0a36;
} SBRecoveredApplicationState;

/* Reconstructs the platform-independent state writes at 1008:ad57- ad60.
 * Framework allocation, text initialization, 1010:d0a6, and 1018:5496 remain
 * explicit boundaries outside this helper. */
void sb_recovered_1008_ac80_0x2af9_state_update(
    SBRecoveredApplicationState *state,
    const SBRecoveredParentObject *object);

#endif

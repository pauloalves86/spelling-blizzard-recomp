#include "application_state.h"

void sb_recovered_1008_ac80_0x2af9_state_update(
    SBRecoveredApplicationState *state,
    const SBRecoveredParentObject *object)
{
    /* Original 1008:ad57-1008:ad60:
     * CALLF 1018:aeaa; INC AX; MOV [01de],AX; MOV byte [0a36],0. */
    state->global_01de = (uint16_t)(
        sb_recovered_1018_aeaa(object) + (uint16_t)1u);
    state->global_0a36 = 0;
}

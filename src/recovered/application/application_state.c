#include "application_state.h"

void sb_recovered_1008_ac80_0x2af9_state_update(
    const SBRecoveredMemory16 *memory,
    SBRecoveredGlobals *globals,
    SBFarPtr16 parent)
{
    /* CALLF 1018:aeaa; INC AX; MOV [01de],AX; MOV byte [0a36],0. */
    globals->word_01de = (uint16_t)(
        sb_recovered_1018_aeaa(memory, parent) + (uint16_t)1u);
    globals->byte_0a36 = 0;
}

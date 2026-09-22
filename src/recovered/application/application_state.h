#ifndef SB_RECOVERED_APPLICATION_STATE_H
#define SB_RECOVERED_APPLICATION_STATE_H

#include "object_model.h"

/* Original 1008:ad57..ad60 in the 0x2af9 path: after the preceding
 * construction/nested-initialization boundary, AX from 1018:aeaa is incremented
 * modulo 2^16 into DAT_1060:01de and DAT_1060:0a36 is cleared. */
void sb_recovered_1008_ac80_0x2af9_state_update(
    const SBRecoveredMemory16 *memory,
    SBRecoveredGlobals *globals,
    SBFarPtr16 parent);

#endif

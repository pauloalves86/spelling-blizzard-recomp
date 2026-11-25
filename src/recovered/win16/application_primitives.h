#ifndef SB_RECOVERED_WIN16_APPLICATION_PRIMITIVES_H
#define SB_RECOVERED_WIN16_APPLICATION_PRIMITIVES_H

#include "../application/object_model.h"

#if defined(__WATCOMC__)
#define SB_WIN16_STDCALL_FAR __far __pascal
#define SB_WIN16_DATA_FAR __far
#else
#define SB_WIN16_STDCALL_FAR
#define SB_WIN16_DATA_FAR
#endif

/* Original Win16 entrypoints: far Pascal ABI and callee stack cleanup. */
uint16_t SB_WIN16_STDCALL_FAR sb_win16_1018_aeaa(
    SBRecoveredParentObject SB_WIN16_DATA_FAR *parent);
void SB_WIN16_STDCALL_FAR sb_win16_1018_aede(
    SBRecoveredParentObject SB_WIN16_DATA_FAR *parent,
    int16_t value);
uint8_t SB_WIN16_STDCALL_FAR sb_win16_1018_af08(
    SBRecoveredParentObject SB_WIN16_DATA_FAR *parent,
    int16_t value);

#undef SB_WIN16_STDCALL_FAR
#undef SB_WIN16_DATA_FAR

#endif

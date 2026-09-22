#ifndef SB_RECOVERED_NESTED_FIELD_H
#define SB_RECOVERED_NESTED_FIELD_H

#include <stdint.h>

/* Derived from BLIZZARD.EXE 1018:aeaa.
 * The original function follows two far pointers: ParentObject + 0x250,
 * then the pointer stored at the referenced holder's offset 0. */
typedef struct SBRecoveredNestedValue {
    uint8_t unknown_00[0x06];
    uint16_t field_06;
} SBRecoveredNestedValue;

typedef struct SBRecoveredNestedHolder {
    SBRecoveredNestedValue *value_at_00;
} SBRecoveredNestedHolder;

typedef struct SBRecoveredParentObject {
    uint8_t unknown_00[0x250];
    SBRecoveredNestedHolder *nested_holder_250;
} SBRecoveredParentObject;

/* Original: 1018:aeaa
 * Calling convention: __stdcall16far, one 4-byte far-pointer parameter,
 * callee cleanup (RETF 0x4), AX return, no explicit register saves. */
uint16_t sb_recovered_1018_aeaa(const SBRecoveredParentObject *object);

#endif

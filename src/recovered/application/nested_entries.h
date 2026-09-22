#ifndef SB_RECOVERED_NESTED_ENTRIES_H
#define SB_RECOVERED_NESTED_ENTRIES_H

#include <stdint.h>

/* Derived from BLIZZARD.EXE 1018:a704.
 * This is a native representation of the valid object graph used by the
 * routine, not a claim about source-level types or ownership. */
enum {
    SB_RECOVERED_1018_A704_ENTRY_CAPACITY = 7,
    SB_RECOVERED_1018_A704_TEXT_BYTES = 6
};

typedef struct SBRecovered1018A704Entry {
    char text_00[SB_RECOVERED_1018_A704_TEXT_BYTES];
    int16_t value_06;
} SBRecovered1018A704Entry;

typedef struct SBRecovered1018A704Record {
    SBRecovered1018A704Entry entries_00[SB_RECOVERED_1018_A704_ENTRY_CAPACITY];
    int16_t count_38;
} SBRecovered1018A704Record;

typedef struct SBRecovered1018A704Holder {
    SBRecovered1018A704Record *record_at_00;
} SBRecovered1018A704Holder;

typedef struct SBRecovered1018A704Parent {
    uint8_t unknown_00[0x250];
    uint32_t holder_far_250;
    uint8_t dirty_254;
    /* Native fixture seam for the 16:16 far pointer at +0x250. */
    SBRecovered1018A704Holder *holder_native;
} SBRecovered1018A704Parent;

/* Original: 1018:a704
 * Calling convention: __stdcall16far; two far parent-pointer parameters;
 * callee cleanup (RETF 0x8). Valid object/string preconditions match the
 * unchecked binary routine. This native model excludes segmented ABI only. */
void sb_recovered_1018_a704(
    SBRecovered1018A704Parent *destination,
    const SBRecovered1018A704Parent *source);

#endif

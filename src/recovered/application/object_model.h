#ifndef SB_RECOVERED_OBJECT_MODEL_H
#define SB_RECOVERED_OBJECT_MODEL_H

#include <stddef.h>
#include <stdint.h>

/* The observed Win16 records have 2-byte structure alignment. */
#pragma pack(push, 2)

/* Binary representation of a Win16 segmented far pointer. The in-memory
 * order is offset then segment, matching the executable's far pointer loads. */
typedef struct SBFarPtr16 {
    uint16_t offset;
    uint16_t segment;
} SBFarPtr16;

/* Final nested object, after following parent +0x250 and holder +0x00. */
typedef struct SBRecoveredNestedSlot {
    uint8_t bytes_00[6];
    int16_t word_06;
} SBRecoveredNestedSlot;

typedef struct SBRecoveredNestedObject {
    SBRecoveredNestedSlot slots_00[7];       /* +0x000..+0x037 */
    int16_t count_38;                         /* +0x038 */
    uint8_t unknown_03a[0x160];               /* +0x03a..+0x199 */
    uint8_t bytes_19a[0x0a];                  /* +0x19a..+0x1a3 */
    int32_t field_1a4;                         /* +0x1a4 */
    int16_t word_1a8;                          /* +0x1a8 */
} SBRecoveredNestedObject;

typedef struct SBRecoveredNestedHolder {
    SBFarPtr16 far_00;                         /* +0x000 */
} SBRecoveredNestedHolder;

/* 0x262-byte replacement object allocated by the known 0x2af9 path. */
typedef struct SBRecoveredParentObject {
    SBFarPtr16 far_000;                         /* +0x000: method table */
    uint8_t unknown_004[0x24c];                /* +0x004..+0x24f */
    SBFarPtr16 far_250;                         /* +0x250 */
    uint8_t byte_254;                           /* +0x254 */
    uint8_t unknown_255;                        /* +0x255 */
    uint8_t unknown_256[0x0c];                 /* +0x256..+0x261 */
} SBRecoveredParentObject;

/* The direct DAT_1060 offsets represented in one neutral image. */
typedef struct SBRecoveredGlobals {
    uint8_t unknown_000[0x01de];
    uint16_t word_01de;                         /* DAT_1060:01de */
    uint8_t unknown_01e0[0x04];
    SBFarPtr16 far_01e4;                        /* DAT_1060:01e4 */
    uint8_t unknown_01e8[0x04];
    SBFarPtr16 far_01ec;                        /* DAT_1060:01ec */
    uint8_t unknown_01f0[0x0846];
    uint8_t byte_0a36;                          /* DAT_1060:0a36 */
    uint8_t unknown_0a37[0x0f];
    SBFarPtr16 far_0a46;                        /* DAT_1060:0a46 */
} SBRecoveredGlobals;

/* Raw 16:16 entries from final method table 1008:2b04. Function meanings
 * remain intentionally unassigned except where recorded beside the entry. */
typedef struct SBRecoveredFinalMethodTable {
    SBFarPtr16 slot_00; SBFarPtr16 slot_04;
    SBFarPtr16 slot_08; SBFarPtr16 slot_0c;
    SBFarPtr16 slot_10; SBFarPtr16 slot_14;
    SBFarPtr16 slot_18; SBFarPtr16 slot_1c; /* 1008:e8b6: RETF 8 */
    SBFarPtr16 slot_20; SBFarPtr16 slot_24;
    SBFarPtr16 slot_28; SBFarPtr16 slot_2c;
    SBFarPtr16 slot_30; SBFarPtr16 slot_34;
    SBFarPtr16 slot_38; SBFarPtr16 slot_3c;
    SBFarPtr16 slot_40; SBFarPtr16 slot_44;
    SBFarPtr16 slot_48; SBFarPtr16 slot_4c;
    SBFarPtr16 slot_50; SBFarPtr16 slot_54;
    SBFarPtr16 slot_58; SBFarPtr16 slot_5c;
    SBFarPtr16 slot_60; SBFarPtr16 slot_64;
    SBFarPtr16 slot_68; /* 1008:e838: RETF 8 */
    SBFarPtr16 slot_6c; SBFarPtr16 slot_70;
} SBRecoveredFinalMethodTable;

extern const SBRecoveredFinalMethodTable sb_recovered_final_method_table_1008_2b04;

#pragma pack(pop)

/* Host-side resolver seam. It maps an explicit 16:16 pointer to the byte at
 * that exact address; it does not replace stored far pointers with host ones. */
typedef uint8_t *(*SBRecoveredResolveFar)(void *context, SBFarPtr16 pointer);
typedef struct SBRecoveredMemory16 {
    void *context;
    SBRecoveredResolveFar resolve;
} SBRecoveredMemory16;

/* Original 1018:aeaa: __stdcall16far, one far parent argument, RETF 4.
 * The native return preserves AX's raw 16-bit representation. */
uint16_t sb_recovered_1018_aeaa(const SBRecoveredMemory16 *memory,
                                SBFarPtr16 parent);

/* Original 1018:a704: __stdcall16far, two far parent arguments, RETF 8. */
void sb_recovered_1018_a704(SBRecoveredMemory16 *memory,
                            SBFarPtr16 destination_parent,
                            SBFarPtr16 source_parent);

/* Original 1018:ae3a: __stdcall16far, far parent + signed word, RETF 6.
 * Its destination is read from globals->far_0a46. */
void sb_recovered_1018_ae3a(SBRecoveredMemory16 *memory,
                            SBRecoveredGlobals *globals,
                            SBFarPtr16 source_parent,
                            int16_t candidate);

/* Original 1018:adb8: __stdcall16far, far parent + flag word + index word,
 * RETF 8. The index remains deliberately unchecked. */
void sb_recovered_1018_adb8(SBRecoveredMemory16 *memory,
                            SBFarPtr16 parent,
                            uint16_t flag,
                            uint16_t index);

/* Native bodies preserve recovered control flow and data operations, while
 * their source comments preserve the original 16-bit far ABI above. */
void sb_recovered_1008_e8b6_noop(void);
void sb_recovered_1008_e838_noop(void);

/* A literal, injectable representation of the known 0x2af9 replacement
 * boundaries. Constructor and nested-initialization internals remain hooks
 * because their complete platform/framework behavior is not natively known. */
typedef struct SBRecoveredReplacementHooks {
    void *context;
    void (*release_slot_00)(void *context, SBFarPtr16 old_parent);
    SBFarPtr16 (*allocate_0262)(void *context);
    void (*construct_before_final_table)(void *context, SBFarPtr16 parent);
    void (*initialize_nested)(void *context, SBFarPtr16 parent);
} SBRecoveredReplacementHooks;

void sb_recovered_1008_ac80_replace_01e4(
    SBRecoveredMemory16 *memory,
    SBRecoveredGlobals *globals,
    const SBRecoveredReplacementHooks *hooks);

_Static_assert(sizeof(SBFarPtr16) == 0x04, "Win16 far pointer must be 4 bytes");
_Static_assert(offsetof(SBFarPtr16, offset) == 0x00, "far offset");
_Static_assert(offsetof(SBFarPtr16, segment) == 0x02, "far segment");
_Static_assert(sizeof(SBRecoveredNestedSlot) == 0x08, "nested slot stride");
_Static_assert(offsetof(SBRecoveredNestedSlot, word_06) == 0x06, "nested slot word");
_Static_assert(offsetof(SBRecoveredNestedObject, count_38) == 0x38, "nested count");
_Static_assert(offsetof(SBRecoveredNestedObject, bytes_19a) == 0x19a, "nested bytes");
_Static_assert(offsetof(SBRecoveredNestedObject, field_1a4) == 0x1a4, "nested field");
_Static_assert(offsetof(SBRecoveredNestedObject, word_1a8) == 0x1a8, "nested word");
_Static_assert(sizeof(SBRecoveredNestedObject) == 0x1aa, "nested object size");
_Static_assert(offsetof(SBRecoveredParentObject, far_250) == 0x250, "parent far pointer");
_Static_assert(offsetof(SBRecoveredParentObject, byte_254) == 0x254, "parent byte");
_Static_assert(offsetof(SBRecoveredParentObject, unknown_256) == 0x256, "parent unknown region");
_Static_assert(sizeof(SBRecoveredParentObject) == 0x262, "parent object size");
_Static_assert(offsetof(SBRecoveredGlobals, word_01de) == 0x01de, "global word");
_Static_assert(offsetof(SBRecoveredGlobals, far_01e4) == 0x01e4, "global far pointer");
_Static_assert(offsetof(SBRecoveredGlobals, far_01ec) == 0x01ec, "global far pointer");
_Static_assert(offsetof(SBRecoveredGlobals, byte_0a36) == 0x0a36, "global byte");
_Static_assert(offsetof(SBRecoveredGlobals, far_0a46) == 0x0a46, "global far pointer");
_Static_assert(offsetof(SBRecoveredFinalMethodTable, slot_1c) == 0x1c, "method slot");
_Static_assert(offsetof(SBRecoveredFinalMethodTable, slot_68) == 0x68, "method slot");

#endif

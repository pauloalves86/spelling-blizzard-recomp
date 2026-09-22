#include "object_model.h"

#include <string.h>

#define SB_FAR(offset_value, segment_value) \
    { .offset = (offset_value), .segment = (segment_value) }

const SBRecoveredFinalMethodTable sb_recovered_final_method_table_1008_2b04 = {
    .slot_00 = SB_FAR(0xaa84, 0x1018), .slot_04 = SB_FAR(0xef9e, 0x1008),
    .slot_08 = SB_FAR(0xefa4, 0x1008), .slot_0c = SB_FAR(0xe838, 0x1008),
    .slot_10 = SB_FAR(0xce88, 0x1008), .slot_14 = SB_FAR(0xe840, 0x1008),
    .slot_18 = SB_FAR(0xe87e, 0x1008), .slot_1c = SB_FAR(0xe8b6, 0x1008),
    .slot_20 = SB_FAR(0xcf6c, 0x1008), .slot_24 = SB_FAR(0xe8be, 0x1008),
    .slot_28 = SB_FAR(0xaee8, 0x1008), .slot_2c = SB_FAR(0xe94a, 0x1008),
    .slot_30 = SB_FAR(0xe988, 0x1008), .slot_34 = SB_FAR(0xe9ac, 0x1008),
    .slot_38 = SB_FAR(0xeb12, 0x1008), .slot_3c = SB_FAR(0xcebc, 0x1008),
    .slot_40 = SB_FAR(0xcf3e, 0x1008), .slot_44 = SB_FAR(0xcfc2, 0x1008),
    .slot_48 = SB_FAR(0xd074, 0x1008), .slot_4c = SB_FAR(0xd0a8, 0x1008),
    .slot_50 = SB_FAR(0xd0ac, 0x1008), .slot_54 = SB_FAR(0xd0f2, 0x1008),
    .slot_58 = SB_FAR(0xd198, 0x1008), .slot_5c = SB_FAR(0xe812, 0x1008),
    .slot_60 = SB_FAR(0xef9e, 0x1008), .slot_64 = SB_FAR(0xefa4, 0x1008),
    .slot_68 = SB_FAR(0xe838, 0x1008), .slot_6c = SB_FAR(0xce88, 0x1008),
    .slot_70 = SB_FAR(0xe840, 0x1008)
};

static uint8_t *resolve(const SBRecoveredMemory16 *memory, SBFarPtr16 pointer)
{
    return memory->resolve(memory->context, pointer);
}

static SBRecoveredParentObject *parent_at(const SBRecoveredMemory16 *memory,
                                          SBFarPtr16 pointer)
{
    return (SBRecoveredParentObject *)resolve(memory, pointer);
}

static SBRecoveredNestedObject *nested_at(const SBRecoveredMemory16 *memory,
                                          SBFarPtr16 parent_pointer)
{
    SBRecoveredParentObject *parent = parent_at(memory, parent_pointer);
    SBRecoveredNestedHolder *holder =
        (SBRecoveredNestedHolder *)resolve(memory, parent->far_250);

    return (SBRecoveredNestedObject *)resolve(memory, holder->far_00);
}

static void copy_slot_bytes(uint8_t destination[6], const uint8_t source[6])
{
    /* 1018:a704 copies through the terminating NUL without a local bounds
     * check. The source/destination precondition intentionally remains that
     * of the binary: a NUL must be reachable. */
    do {
        *destination = *source;
    } while (*source++ != 0 && ++destination);
}

static int slot_bytes_equal(const uint8_t left[6], const uint8_t right[6])
{
    while (*left == *right) {
        if (*left == 0) {
            return 1;
        }
        ++left;
        ++right;
    }
    return 0;
}

static int32_t sign_extend_16(uint16_t value)
{
    if ((value & 0x8000u) != 0u) {
        return (int32_t)(value | 0xffff0000u);
    }
    return (int32_t)value;
}

uint16_t sb_recovered_1018_aeaa(const SBRecoveredMemory16 *memory,
                                SBFarPtr16 parent)
{
    return (uint16_t)nested_at(memory, parent)->slots_00[0].word_06;
}

void sb_recovered_1018_a704(SBRecoveredMemory16 *memory,
                            SBFarPtr16 destination_parent,
                            SBFarPtr16 source_parent)
{
    SBRecoveredParentObject *destination_object = parent_at(memory, destination_parent);
    SBRecoveredNestedObject *destination = nested_at(memory, destination_parent);
    SBRecoveredNestedObject *source = nested_at(memory, source_parent);
    uint8_t source_bytes_00[6];
    int16_t source_word_06 = source->slots_00[0].word_06;
    int16_t index;
    int16_t rank = 0;

    copy_slot_bytes(source_bytes_00, source->slots_00[0].bytes_00);

    for (index = 0; index < destination->count_38; ++index) {
        if (slot_bytes_equal(destination->slots_00[index].bytes_00, source_bytes_00)) {
            int16_t shift;
            for (shift = (int16_t)(index + 1); shift < destination->count_38; ++shift) {
                copy_slot_bytes(destination->slots_00[shift - 1].bytes_00,
                                destination->slots_00[shift].bytes_00);
                destination->slots_00[shift - 1].word_06 =
                    destination->slots_00[shift].word_06;
            }
            --destination->count_38;
            break;
        }
    }

    for (index = 0; index < destination->count_38; ++index) {
        if (source_word_06 < destination->slots_00[index].word_06) {
            ++rank;
        }
    }

    if (rank < 7) {
        for (index = 6; index > rank; --index) {
            copy_slot_bytes(destination->slots_00[index].bytes_00,
                            destination->slots_00[index - 1].bytes_00);
            destination->slots_00[index].word_06 =
                destination->slots_00[index - 1].word_06;
        }
        copy_slot_bytes(destination->slots_00[rank].bytes_00, source_bytes_00);
        destination->slots_00[rank].word_06 = source_word_06;
        ++destination->count_38;
        if (destination->count_38 > 7) {
            destination->count_38 = 7;
        }
    }

    destination_object->byte_254 = 1;
}

void sb_recovered_1018_ae3a(SBRecoveredMemory16 *memory,
                            SBRecoveredGlobals *globals,
                            SBFarPtr16 source_parent,
                            int16_t candidate)
{
    SBRecoveredParentObject *source_object = parent_at(memory, source_parent);
    SBRecoveredNestedObject *source = nested_at(memory, source_parent);

    if (source->slots_00[0].word_06 < candidate) {
        source->slots_00[0].word_06 = candidate;
    }
    source_object->byte_254 = 1;
    sb_recovered_1018_a704(memory, globals->far_0a46, source_parent);
}

void sb_recovered_1018_adb8(SBRecoveredMemory16 *memory,
                            SBFarPtr16 parent,
                            uint16_t flag,
                            uint16_t index)
{
    SBRecoveredParentObject *parent_object = parent_at(memory, parent);
    SBRecoveredNestedObject *nested = nested_at(memory, parent);
    uint16_t selected_offset = (uint16_t)(0x08u +
        (uint16_t)(2u * (uint16_t)(index - 1u)));
    SBFarPtr16 selected_pointer = {0};
    int16_t *selected;
    uint16_t old_raw;
    uint16_t new_raw;
    int32_t candidate;
    uint32_t accumulated;

    /* The pointer arithmetic is intentionally modulo 2^16. Use the nested
     * object's actual far address reconstructed from the holder, not a host
     * pointer, so an adapter can expose the original out-of-range behavior. */
    {
        SBRecoveredNestedHolder *holder =
            (SBRecoveredNestedHolder *)resolve(memory, parent_object->far_250);
        selected_pointer.offset = (uint16_t)(holder->far_00.offset + selected_offset);
        selected_pointer.segment = holder->far_00.segment;
    }
    selected = (int16_t *)resolve(memory, selected_pointer);
    old_raw = (uint16_t)*selected;
    candidate = sign_extend_16(old_raw) + ((flag & 0xffu) == 0u ? 40 : -20);
    if (candidate < 1) {
        candidate = 1;
    } else if (candidate > 200) {
        candidate = 200;
    }
    new_raw = (uint16_t)candidate;
    *selected = (int16_t)new_raw;

    memcpy(&accumulated, &nested->field_1a4, sizeof(accumulated));
    accumulated += (uint32_t)sign_extend_16((uint16_t)(new_raw - old_raw));
    memcpy(&nested->field_1a4, &accumulated, sizeof(accumulated));
    parent_object->byte_254 = 1;
}

void sb_recovered_1008_e8b6_noop(void)
{
}

void sb_recovered_1008_e838_noop(void)
{
}

void sb_recovered_1008_ac80_replace_01e4(
    SBRecoveredMemory16 *memory,
    SBRecoveredGlobals *globals,
    const SBRecoveredReplacementHooks *hooks)
{
    SBFarPtr16 replacement;
    SBRecoveredParentObject *replacement_object;

    if (globals->far_01e4.offset != 0 || globals->far_01e4.segment != 0) {
        hooks->release_slot_00(hooks->context, globals->far_01e4);
    }
    replacement = hooks->allocate_0262(hooks->context);
    hooks->construct_before_final_table(hooks->context, replacement);
    replacement_object = parent_at(memory, replacement);
    replacement_object->far_000 = (SBFarPtr16)SB_FAR(0x2b04, 0x1008);
    hooks->initialize_nested(hooks->context, replacement);
    globals->far_01e4 = replacement;
}

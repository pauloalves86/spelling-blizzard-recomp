#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "application_state.h"
#include "object_model.h"

typedef struct TestSegment {
    uint16_t segment;
    uint8_t bytes[UINT16_MAX + 1u];
} TestSegment;

static uint8_t *resolve_test_segment(void *context, SBFarPtr16 pointer)
{
    TestSegment *segment = context;

    assert(pointer.segment == segment->segment);
    return &segment->bytes[pointer.offset];
}

static SBFarPtr16 far_at(uint16_t offset, uint16_t segment)
{
    return (SBFarPtr16){.offset = offset, .segment = segment};
}

static void set_slot(SBRecoveredNestedSlot *slot, const char *bytes, int16_t word)
{
    memset(slot->bytes_00, 0, sizeof(slot->bytes_00));
    memcpy(slot->bytes_00, bytes, strlen(bytes) + 1u);
    slot->word_06 = word;
}

static void setup_parent_chain(TestSegment *segment,
                               uint16_t parent_offset,
                               uint16_t holder_offset,
                               uint16_t nested_offset)
{
    SBRecoveredParentObject *parent =
        (SBRecoveredParentObject *)&segment->bytes[parent_offset];
    SBRecoveredNestedHolder *holder =
        (SBRecoveredNestedHolder *)&segment->bytes[holder_offset];

    parent->far_250 = far_at(holder_offset, segment->segment);
    holder->far_00 = far_at(nested_offset, segment->segment);
}

static void test_accessors_and_mutations(void)
{
    TestSegment segment = {.segment = 0x1234};
    SBRecoveredMemory16 memory = {
        .context = &segment,
        .resolve = resolve_test_segment
    };
    SBFarPtr16 source_parent = far_at(0x0100, segment.segment);
    SBFarPtr16 destination_parent = far_at(0x0400, segment.segment);
    SBFarPtr16 source_nested = far_at(0x0c00, segment.segment);
    SBFarPtr16 destination_nested = far_at(0x1000, segment.segment);
    SBRecoveredNestedObject *source =
        (SBRecoveredNestedObject *)&segment.bytes[source_nested.offset];
    SBRecoveredNestedObject *destination =
        (SBRecoveredNestedObject *)&segment.bytes[destination_nested.offset];
    SBRecoveredParentObject *source_object =
        (SBRecoveredParentObject *)&segment.bytes[source_parent.offset];
    SBRecoveredParentObject *destination_object =
        (SBRecoveredParentObject *)&segment.bytes[destination_parent.offset];
    SBRecoveredGlobals *globals = (SBRecoveredGlobals *)&segment.bytes[0x2000];

    setup_parent_chain(&segment, source_parent.offset, 0x0800, source_nested.offset);
    setup_parent_chain(&segment, destination_parent.offset, 0x0a00, destination_nested.offset);
    set_slot(&source->slots_00[0], "S", 50);
    set_slot(&destination->slots_00[0], "A", 80);
    set_slot(&destination->slots_00[1], "B", 40);
    destination->count_38 = 2;
    globals->far_0a46 = destination_parent;

    assert(sb_recovered_1018_aeaa(&memory, source_parent) == 50);
    globals->word_01de = 0xffff;
    globals->byte_0a36 = 0xff;
    sb_recovered_1008_ac80_0x2af9_state_update(&memory, globals, source_parent);
    assert(globals->word_01de == 51);
    assert(globals->byte_0a36 == 0);
    sb_recovered_1018_ae3a(&memory, globals, source_parent, 60);
    assert(source->slots_00[0].word_06 == 60);
    assert(source_object->byte_254 == 1);
    assert(destination_object->byte_254 == 1);
    assert(destination->count_38 == 3);
    assert(destination->slots_00[0].word_06 == 80);
    assert(destination->slots_00[1].word_06 == 60);
    assert(destination->slots_00[2].word_06 == 40);
}

static void test_aeaa_raw_values(void)
{
    static const uint16_t values[] = {0, 1, 0x1234, UINT16_MAX};
    TestSegment segment = {.segment = 0x1234};
    SBRecoveredMemory16 memory = {
        .context = &segment,
        .resolve = resolve_test_segment
    };
    SBFarPtr16 first_parent = far_at(0x0100, segment.segment);
    SBFarPtr16 second_parent = far_at(0x0400, segment.segment);
    SBRecoveredNestedObject *first =
        (SBRecoveredNestedObject *)&segment.bytes[0x0c00];
    SBRecoveredNestedObject *second =
        (SBRecoveredNestedObject *)&segment.bytes[0x1000];
    size_t index;

    setup_parent_chain(&segment, first_parent.offset, 0x0800, 0x0c00);
    setup_parent_chain(&segment, second_parent.offset, 0x0a00, 0x1000);
    for (index = 0; index < sizeof(values) / sizeof(values[0]); ++index) {
        first->slots_00[0].word_06 = (int16_t)values[index];
        assert(sb_recovered_1018_aeaa(&memory, first_parent) == values[index]);
    }
    first->slots_00[0].word_06 = 7;
    second->slots_00[0].word_06 = 99;
    assert(sb_recovered_1018_aeaa(&memory, first_parent) == 7);
    assert(sb_recovered_1018_aeaa(&memory, second_parent) == 99);
}

static void test_aede_word_store_and_dirty_byte(void)
{
    TestSegment segment = {.segment = 0x1234};
    SBRecoveredMemory16 memory = {
        .context = &segment,
        .resolve = resolve_test_segment
    };
    SBFarPtr16 parent = far_at(0x0100, segment.segment);
    SBRecoveredParentObject *parent_object =
        (SBRecoveredParentObject *)&segment.bytes[parent.offset];
    SBRecoveredNestedObject *nested =
        (SBRecoveredNestedObject *)&segment.bytes[0x0c00];

    setup_parent_chain(&segment, parent.offset, 0x0800, 0x0c00);
    nested->word_1a8 = 17;
    parent_object->byte_254 = 0;

    sb_recovered_1018_aede(&memory, parent, (int16_t)0x8123);

    assert((uint16_t)nested->word_1a8 == 0x8123);
    assert(parent_object->byte_254 == 1);
}

static void test_ordered_slots(void)
{
    int count;

    for (count = 0; count <= 7; ++count) {
        TestSegment segment = {.segment = 0x1234};
        SBRecoveredMemory16 memory = {
            .context = &segment,
            .resolve = resolve_test_segment
        };
        SBFarPtr16 source_parent = far_at(0x0100, segment.segment);
        SBFarPtr16 destination_parent = far_at(0x0400, segment.segment);
        SBRecoveredNestedObject *source =
            (SBRecoveredNestedObject *)&segment.bytes[0x0c00];
        SBRecoveredNestedObject *destination =
            (SBRecoveredNestedObject *)&segment.bytes[0x1000];
        int index;

        setup_parent_chain(&segment, source_parent.offset, 0x0800, 0x0c00);
        setup_parent_chain(&segment, destination_parent.offset, 0x0a00, 0x1000);
        for (index = 0; index < count; ++index) {
            char byte[2] = {(char)('A' + index), 0};
            set_slot(&destination->slots_00[index], byte, (int16_t)(70 - index));
        }
        destination->count_38 = (int16_t)count;
        set_slot(&source->slots_00[0], "Z", -1);

        sb_recovered_1018_a704(&memory, destination_parent, source_parent);
        assert(destination->count_38 == (count == 7 ? 7 : count + 1));
    }

    {
        TestSegment segment = {.segment = 0x1234};
        SBRecoveredMemory16 memory = {
            .context = &segment,
            .resolve = resolve_test_segment
        };
        SBFarPtr16 source_parent = far_at(0x0100, segment.segment);
        SBFarPtr16 destination_parent = far_at(0x0400, segment.segment);
        SBRecoveredNestedObject *source =
            (SBRecoveredNestedObject *)&segment.bytes[0x0c00];
        SBRecoveredNestedObject *destination =
            (SBRecoveredNestedObject *)&segment.bytes[0x1000];

        setup_parent_chain(&segment, source_parent.offset, 0x0800, 0x0c00);
        setup_parent_chain(&segment, destination_parent.offset, 0x0a00, 0x1000);
        set_slot(&destination->slots_00[0], "A", 90);
        set_slot(&destination->slots_00[1], "X", 50);
        set_slot(&destination->slots_00[2], "X", 40);
        destination->count_38 = 3;
        set_slot(&source->slots_00[0], "X", 50);

        sb_recovered_1018_a704(&memory, destination_parent, source_parent);
        assert(destination->count_38 == 3);
        assert(destination->slots_00[0].word_06 == 90);
        assert(destination->slots_00[1].word_06 == 50);
        assert(destination->slots_00[2].word_06 == 40);
        assert(destination->slots_00[1].bytes_00[0] == 'X');
        assert(destination->slots_00[2].bytes_00[0] == 'X');
    }
}

static void test_1018_adb8_signed_clamp_and_accumulation(void)
{
    TestSegment segment = {.segment = 0x1234};
    SBRecoveredMemory16 memory = {
        .context = &segment,
        .resolve = resolve_test_segment
    };
    SBFarPtr16 parent = far_at(0x0100, segment.segment);
    SBFarPtr16 nested = far_at(0x0c00, segment.segment);
    SBRecoveredNestedObject *nested_object =
        (SBRecoveredNestedObject *)&segment.bytes[nested.offset];
    SBRecoveredParentObject *parent_object =
        (SBRecoveredParentObject *)&segment.bytes[parent.offset];
    int16_t *selected_word = (int16_t *)&segment.bytes[nested.offset + 0x08];

    setup_parent_chain(&segment, parent.offset, 0x0800, nested.offset);
    *selected_word = 190;
    nested_object->field_1a4 = 1000;
    sb_recovered_1018_adb8(&memory, parent, 0, 1);
    assert(*selected_word == 200);
    assert(nested_object->field_1a4 == 1010);
    assert(parent_object->byte_254 == 1);

    *selected_word = 5;
    sb_recovered_1018_adb8(&memory, parent, 1, 1);
    assert(*selected_word == 1);
    assert(nested_object->field_1a4 == 1006);
}

typedef struct LifecycleLog {
    uint16_t segment;
    unsigned int calls[3];
} LifecycleLog;

static void lifecycle_release(void *context, SBFarPtr16 old_parent)
{
    LifecycleLog *log = context;
    assert(old_parent.offset == 0x0100);
    log->calls[0] = 1;
}

static SBFarPtr16 lifecycle_allocate(void *context)
{
    LifecycleLog *log = context;
    log->calls[1] = log->calls[0] + 1;
    return far_at(0x3000, log->segment);
}

static void lifecycle_construct(void *context, SBFarPtr16 parent)
{
    LifecycleLog *log = context;
    assert(parent.offset == 0x3000);
    log->calls[2] = log->calls[1] + 1;
}

static void lifecycle_initialize_nested(void *context, SBFarPtr16 parent)
{
    LifecycleLog *log = context;
    assert(parent.offset == 0x3000);
    assert(log->calls[2] == 3);
    log->calls[2] = 4;
}

static void test_known_replacement_boundaries(void)
{
    TestSegment segment = {.segment = 0x1234};
    SBRecoveredMemory16 memory = {
        .context = &segment,
        .resolve = resolve_test_segment
    };
    SBRecoveredGlobals *globals = (SBRecoveredGlobals *)&segment.bytes[0x2000];
    LifecycleLog log = {.segment = segment.segment};
    SBRecoveredReplacementHooks hooks = {
        .context = &log,
        .release_slot_00 = lifecycle_release,
        .allocate_0262 = lifecycle_allocate,
        .construct_before_final_table = lifecycle_construct,
        .initialize_nested = lifecycle_initialize_nested
    };
    SBRecoveredParentObject *replacement =
        (SBRecoveredParentObject *)&segment.bytes[0x3000];

    globals->far_01e4 = far_at(0x0100, segment.segment);
    sb_recovered_1008_ac80_replace_01e4(&memory, globals, &hooks);

    assert(log.calls[0] == 1);
    assert(log.calls[1] == 2);
    assert(log.calls[2] == 4);
    assert(globals->far_01e4.offset == 0x3000);
    assert(replacement->far_000.offset == 0x2b04);
    assert(replacement->far_000.segment == 0x1008);
}

int main(void)
{
    assert(sizeof(SBFarPtr16) == 0x04);
    assert(offsetof(SBFarPtr16, offset) == 0x00);
    assert(offsetof(SBFarPtr16, segment) == 0x02);

    assert(sizeof(SBRecoveredNestedSlot) == 0x08);
    assert(offsetof(SBRecoveredNestedSlot, word_06) == 0x06);
    assert(offsetof(SBRecoveredNestedObject, slots_00) == 0x00);
    assert(offsetof(SBRecoveredNestedObject, count_38) == 0x38);
    assert(offsetof(SBRecoveredNestedObject, bytes_19a) == 0x19a);
    assert(offsetof(SBRecoveredNestedObject, field_1a4) == 0x1a4);
    assert(offsetof(SBRecoveredNestedObject, word_1a8) == 0x1a8);
    assert(sizeof(SBRecoveredNestedObject) == 0x1aa);

    assert(offsetof(SBRecoveredParentObject, far_000) == 0x000);
    assert(offsetof(SBRecoveredParentObject, far_250) == 0x250);
    assert(offsetof(SBRecoveredParentObject, byte_254) == 0x254);
    assert(offsetof(SBRecoveredParentObject, unknown_256) == 0x256);
    assert(sizeof(SBRecoveredParentObject) == 0x262);

    assert(offsetof(SBRecoveredGlobals, word_01de) == 0x01de);
    assert(offsetof(SBRecoveredGlobals, far_01e4) == 0x01e4);
    assert(offsetof(SBRecoveredGlobals, far_01ec) == 0x01ec);
    assert(offsetof(SBRecoveredGlobals, byte_0a36) == 0x0a36);
    assert(offsetof(SBRecoveredGlobals, far_0a46) == 0x0a46);
    assert(offsetof(SBRecoveredFinalMethodTable, slot_1c) == 0x1c);
    assert(offsetof(SBRecoveredFinalMethodTable, slot_68) == 0x68);
    assert(sb_recovered_final_method_table_1008_2b04.slot_00.offset == 0xaa84);
    assert(sb_recovered_final_method_table_1008_2b04.slot_00.segment == 0x1018);
    assert(sb_recovered_final_method_table_1008_2b04.slot_1c.offset == 0xe8b6);
    assert(sb_recovered_final_method_table_1008_2b04.slot_68.offset == 0xe838);

    test_accessors_and_mutations();
    test_aeaa_raw_values();
    test_aede_word_store_and_dirty_byte();
    test_ordered_slots();
    test_1018_adb8_signed_clamp_and_accumulation();
    test_known_replacement_boundaries();
    return 0;
}

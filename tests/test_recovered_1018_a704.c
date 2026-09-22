#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "nested_entries.h"

static void set_entry(SBRecovered1018A704Entry *entry,
                      const char *text,
                      int16_t value)
{
    strcpy(entry->text_00, text);
    entry->value_06 = value;
}

static void make_parent(SBRecovered1018A704Parent *parent,
                        SBRecovered1018A704Holder *holder,
                        SBRecovered1018A704Record *record)
{
    *parent = (SBRecovered1018A704Parent){0};
    *holder = (SBRecovered1018A704Holder){.record_at_00 = record};
    parent->holder_native = holder;
}

static void test_descending_insert_and_tie_precedence(void)
{
    SBRecovered1018A704Record destination = {0};
    SBRecovered1018A704Record source = {0};
    SBRecovered1018A704Holder destination_holder;
    SBRecovered1018A704Holder source_holder;
    SBRecovered1018A704Parent destination_parent;
    SBRecovered1018A704Parent source_parent;

    set_entry(&destination.entries_00[0], "A", 90);
    set_entry(&destination.entries_00[1], "B", 50);
    destination.count_38 = 2;
    set_entry(&source.entries_00[0], "C", 50);
    make_parent(&destination_parent, &destination_holder, &destination);
    make_parent(&source_parent, &source_holder, &source);

    sb_recovered_1018_a704(&destination_parent, &source_parent);

    assert(destination.count_38 == 3);
    assert(strcmp(destination.entries_00[0].text_00, "A") == 0);
    assert(strcmp(destination.entries_00[1].text_00, "C") == 0);
    assert(strcmp(destination.entries_00[2].text_00, "B") == 0);
    assert(destination_parent.dirty_254 == 1);
}

static void test_first_duplicate_removed_before_insert(void)
{
    SBRecovered1018A704Record destination = {0};
    SBRecovered1018A704Record source = {0};
    SBRecovered1018A704Holder destination_holder;
    SBRecovered1018A704Holder source_holder;
    SBRecovered1018A704Parent destination_parent;
    SBRecovered1018A704Parent source_parent;

    set_entry(&destination.entries_00[0], "X", 90);
    set_entry(&destination.entries_00[1], "X", 40);
    set_entry(&destination.entries_00[2], "Y", 30);
    destination.count_38 = 3;
    set_entry(&source.entries_00[0], "X", 70);
    make_parent(&destination_parent, &destination_holder, &destination);
    make_parent(&source_parent, &source_holder, &source);

    sb_recovered_1018_a704(&destination_parent, &source_parent);

    assert(destination.count_38 == 3);
    assert(destination.entries_00[0].value_06 == 70);
    assert(destination.entries_00[1].value_06 == 40);
    assert(destination.entries_00[2].value_06 == 30);
}

static void test_seven_entry_cap_and_dirty_write(void)
{
    SBRecovered1018A704Record destination = {0};
    SBRecovered1018A704Record source = {0};
    SBRecovered1018A704Holder destination_holder;
    SBRecovered1018A704Holder source_holder;
    SBRecovered1018A704Parent destination_parent;
    SBRecovered1018A704Parent source_parent;
    int index;

    for (index = 0; index < SB_RECOVERED_1018_A704_ENTRY_CAPACITY; ++index) {
        destination.entries_00[index].text_00[0] = (char)('A' + index);
        destination.entries_00[index].text_00[1] = '\0';
        destination.entries_00[index].value_06 = (int16_t)(70 - index);
    }
    destination.count_38 = SB_RECOVERED_1018_A704_ENTRY_CAPACITY;
    set_entry(&source.entries_00[0], "Z", -1);
    make_parent(&destination_parent, &destination_holder, &destination);
    make_parent(&source_parent, &source_holder, &source);

    sb_recovered_1018_a704(&destination_parent, &source_parent);

    assert(destination.count_38 == SB_RECOVERED_1018_A704_ENTRY_CAPACITY);
    assert(strcmp(destination.entries_00[6].text_00, "G") == 0);
    assert(destination_parent.dirty_254 == 1);
}

static void test_signed_extremes(void)
{
    SBRecovered1018A704Record destination = {0};
    SBRecovered1018A704Record source = {0};
    SBRecovered1018A704Holder destination_holder;
    SBRecovered1018A704Holder source_holder;
    SBRecovered1018A704Parent destination_parent;
    SBRecovered1018A704Parent source_parent;

    set_entry(&destination.entries_00[0], "N", -1);
    set_entry(&destination.entries_00[1], "L", INT16_MIN);
    destination.count_38 = 2;
    set_entry(&source.entries_00[0], "H", INT16_MAX);
    make_parent(&destination_parent, &destination_holder, &destination);
    make_parent(&source_parent, &source_holder, &source);

    sb_recovered_1018_a704(&destination_parent, &source_parent);

    assert(destination.count_38 == 3);
    assert(destination.entries_00[0].value_06 == INT16_MAX);
    assert(destination.entries_00[1].value_06 == -1);
    assert(destination.entries_00[2].value_06 == INT16_MIN);
}

static void test_duplicate_replacement_when_full(void)
{
    SBRecovered1018A704Record destination = {0};
    SBRecovered1018A704Record source = {0};
    SBRecovered1018A704Holder destination_holder;
    SBRecovered1018A704Holder source_holder;
    SBRecovered1018A704Parent destination_parent;
    SBRecovered1018A704Parent source_parent;
    int index;

    for (index = 0; index < SB_RECOVERED_1018_A704_ENTRY_CAPACITY; ++index) {
        destination.entries_00[index].text_00[0] = (char)('A' + index);
        destination.entries_00[index].text_00[1] = '\0';
        destination.entries_00[index].value_06 = (int16_t)(70 - index * 10);
    }
    destination.count_38 = SB_RECOVERED_1018_A704_ENTRY_CAPACITY;
    set_entry(&source.entries_00[0], "D", 75);
    make_parent(&destination_parent, &destination_holder, &destination);
    make_parent(&source_parent, &source_holder, &source);

    sb_recovered_1018_a704(&destination_parent, &source_parent);

    assert(destination.count_38 == SB_RECOVERED_1018_A704_ENTRY_CAPACITY);
    assert(strcmp(destination.entries_00[0].text_00, "D") == 0);
    assert(destination.entries_00[0].value_06 == 75);
    assert(strcmp(destination.entries_00[6].text_00, "G") == 0);
}

static void test_duplicate_in_final_active_slot(void)
{
    SBRecovered1018A704Record destination = {0};
    SBRecovered1018A704Record source = {0};
    SBRecovered1018A704Holder destination_holder;
    SBRecovered1018A704Holder source_holder;
    SBRecovered1018A704Parent destination_parent;
    SBRecovered1018A704Parent source_parent;
    int index;

    for (index = 0; index < SB_RECOVERED_1018_A704_ENTRY_CAPACITY; ++index) {
        destination.entries_00[index].text_00[0] = (char)('A' + index);
        destination.entries_00[index].text_00[1] = '\0';
        destination.entries_00[index].value_06 = (int16_t)(70 - index * 10);
    }
    destination.count_38 = SB_RECOVERED_1018_A704_ENTRY_CAPACITY;
    set_entry(&source.entries_00[0], "G", 65);
    make_parent(&destination_parent, &destination_holder, &destination);
    make_parent(&source_parent, &source_holder, &source);

    sb_recovered_1018_a704(&destination_parent, &source_parent);

    assert(destination.count_38 == SB_RECOVERED_1018_A704_ENTRY_CAPACITY);
    assert(strcmp(destination.entries_00[0].text_00, "A") == 0);
    assert(strcmp(destination.entries_00[1].text_00, "G") == 0);
    assert(destination.entries_00[1].value_06 == 65);
}

static void test_active_count_range_zero_through_seven(void)
{
    int count;

    for (count = 0; count <= SB_RECOVERED_1018_A704_ENTRY_CAPACITY; ++count) {
        SBRecovered1018A704Record destination = {0};
        SBRecovered1018A704Record source = {0};
        SBRecovered1018A704Holder destination_holder;
        SBRecovered1018A704Holder source_holder;
        SBRecovered1018A704Parent destination_parent;
        SBRecovered1018A704Parent source_parent;
        int index;

        for (index = 0; index < count; ++index) {
            destination.entries_00[index].text_00[0] = (char)('A' + index);
            destination.entries_00[index].text_00[1] = '\0';
            destination.entries_00[index].value_06 = (int16_t)(60 - index * 10);
        }
        destination.count_38 = (int16_t)count;
        set_entry(&source.entries_00[0], "Z", 0);
        make_parent(&destination_parent, &destination_holder, &destination);
        make_parent(&source_parent, &source_holder, &source);

        sb_recovered_1018_a704(&destination_parent, &source_parent);

        assert(destination.count_38 ==
               (count == SB_RECOVERED_1018_A704_ENTRY_CAPACITY ? count : count + 1));
        assert(destination_parent.dirty_254 == 1);
    }
}

int main(void)
{
    assert(sizeof(SBRecovered1018A704Entry) == 0x08);
    assert(offsetof(SBRecovered1018A704Entry, value_06) == 0x06);
    assert(offsetof(SBRecovered1018A704Record, count_38) == 0x38);
    assert(offsetof(SBRecovered1018A704Parent, holder_far_250) == 0x250);
    assert(offsetof(SBRecovered1018A704Parent, dirty_254) == 0x254);

    test_descending_insert_and_tie_precedence();
    test_first_duplicate_removed_before_insert();
    test_seven_entry_cap_and_dirty_write();
    test_signed_extremes();
    test_duplicate_replacement_when_full();
    test_duplicate_in_final_active_slot();
    test_active_count_range_zero_through_seven();
    return 0;
}

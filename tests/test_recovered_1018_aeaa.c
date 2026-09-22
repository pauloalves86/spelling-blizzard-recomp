#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "nested_field.h"

static void test_values(void)
{
    assert(offsetof(SBRecoveredParentObject, nested_holder_250) == 0x250);
    assert(offsetof(SBRecoveredNestedValue, field_06) == 0x06);
    SBRecoveredNestedValue values[4] = {
        {.field_06 = 0},
        {.field_06 = 1},
        {.field_06 = 0x1234u},
        {.field_06 = UINT16_MAX}
    };
    SBRecoveredNestedHolder holders[4];
    SBRecoveredParentObject parents[4] = {0};

    for (size_t i = 0; i < 4; ++i) {
        holders[i].value_at_00 = &values[i];
        parents[i].nested_holder_250 = &holders[i];
        assert(sb_recovered_1018_aeaa(&parents[i]) == values[i].field_06);
    }
}

static void test_independent_instances(void)
{
    SBRecoveredNestedValue first_value = {.field_06 = 7};
    SBRecoveredNestedValue second_value = {.field_06 = 99};
    SBRecoveredNestedHolder first_holder = {.value_at_00 = &first_value};
    SBRecoveredNestedHolder second_holder = {.value_at_00 = &second_value};
    SBRecoveredParentObject first = {.nested_holder_250 = &first_holder};
    SBRecoveredParentObject second = {.nested_holder_250 = &second_holder};

    assert(sb_recovered_1018_aeaa(&first) == 7);
    assert(sb_recovered_1018_aeaa(&second) == 99);
}

int main(void)
{
    test_values();
    test_independent_instances();
    return 0;
}

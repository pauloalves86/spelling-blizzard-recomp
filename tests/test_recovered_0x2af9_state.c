#include <assert.h>
#include <stdint.h>
#include "application_state.h"

static void run_case(uint16_t field_value, uint16_t expected)
{
    SBRecoveredNestedValue value = {.field_06 = field_value};
    SBRecoveredNestedHolder holder = {.value_at_00 = &value};
    SBRecoveredParentObject object = {.nested_holder_250 = &holder};
    SBRecoveredApplicationState state = {
        .global_01de = 0xaaaa,
        .global_0a36 = 0xff
    };

    sb_recovered_1008_ac80_0x2af9_state_update(&state, &object);
    assert(state.global_01de == expected);
    assert(state.global_0a36 == 0);
}

int main(void)
{
    run_case(0, 1);
    run_case(99, 100);
    run_case(UINT16_MAX, 0);
    return 0;
}

#include "application_primitives.h"
#include "../generated/classes_vtables.h"

#include <i86.h>

static SBRecoveredNestedObject __far *nested_object_at(
    SBRecoveredParentObject __far *parent)
{
    SBRecoveredNestedHolder __far *holder;

    holder = (SBRecoveredNestedHolder __far *)MK_FP(
        parent->far_250.segment, parent->far_250.offset);
    return (SBRecoveredNestedObject __far *)MK_FP(
        holder->far_00.segment, holder->far_00.offset);
}

uint16_t __far __pascal sb_win16_1018_aeaa(
    SBRecoveredParentObject __far *parent)
{
    SBRecoveredNestedObject __far *nested = nested_object_at(parent);
    return (uint16_t)nested->slots_00[0].word_06;
}

void __far __pascal sb_win16_1018_aede(
    SBRecoveredParentObject __far *parent,
    int16_t value)
{
    SBRecoveredNestedObject __far *nested = nested_object_at(parent);
    nested->word_1a8 = value;
    parent->byte_254 = 1;
}

uint8_t __far __pascal sb_win16_1018_af08(
    SBRecoveredParentObject __far *parent,
    int16_t value)
{
    SBRecoveredNestedObject __far *nested = nested_object_at(parent);
    return (uint8_t)(value <= nested->word_1a8);
}

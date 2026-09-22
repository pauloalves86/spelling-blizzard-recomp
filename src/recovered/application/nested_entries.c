#include "nested_entries.h"

#include <string.h>

static void copy_text_00(char destination[SB_RECOVERED_1018_A704_TEXT_BYTES],
                         const char *source)
{
    /* Original 1018:a71e..a741 and its shift/insert copies scan to NUL with
     * no explicit local-buffer bound. Callers must satisfy that precondition. */
    do {
        *destination = *source;
    } while (*source++ != '\0' && ++destination);
}

void sb_recovered_1018_a704(
    SBRecovered1018A704Parent *destination,
    const SBRecovered1018A704Parent *source)
{
    SBRecovered1018A704Record *destination_record =
        destination->holder_native->record_at_00;
    const SBRecovered1018A704Record *source_record =
        source->holder_native->record_at_00;
    char text_00[SB_RECOVERED_1018_A704_TEXT_BYTES];
    int16_t source_value_06 = source_record->entries_00[0].value_06;
    int16_t index;
    int16_t insertion_rank = 0;

    copy_text_00(text_00, source_record->entries_00[0].text_00);

    for (index = 0; index < destination_record->count_38; ++index) {
        if (strcmp(destination_record->entries_00[index].text_00, text_00) == 0) {
            int16_t shift;

            for (shift = (int16_t)(index + 1);
                 shift < destination_record->count_38;
                 ++shift) {
                copy_text_00(destination_record->entries_00[shift - 1].text_00,
                             destination_record->entries_00[shift].text_00);
                destination_record->entries_00[shift - 1].value_06 =
                    destination_record->entries_00[shift].value_06;
            }
            --destination_record->count_38;
            break;
        }
    }

    for (index = 0; index < destination_record->count_38; ++index) {
        if (source_value_06 < destination_record->entries_00[index].value_06) {
            ++insertion_rank;
        }
    }

    if (insertion_rank < SB_RECOVERED_1018_A704_ENTRY_CAPACITY) {
        for (index = SB_RECOVERED_1018_A704_ENTRY_CAPACITY - 1;
             index > insertion_rank;
             --index) {
            copy_text_00(destination_record->entries_00[index].text_00,
                         destination_record->entries_00[index - 1].text_00);
            destination_record->entries_00[index].value_06 =
                destination_record->entries_00[index - 1].value_06;
        }

        copy_text_00(destination_record->entries_00[insertion_rank].text_00,
                     text_00);
        destination_record->entries_00[insertion_rank].value_06 = source_value_06;

        ++destination_record->count_38;
        if (destination_record->count_38 > SB_RECOVERED_1018_A704_ENTRY_CAPACITY) {
            destination_record->count_38 = SB_RECOVERED_1018_A704_ENTRY_CAPACITY;
        }
    }

    destination->dirty_254 = 1;
}

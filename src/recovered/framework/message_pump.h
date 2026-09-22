#ifndef SB_RECOVERED_MESSAGE_PUMP_H
#define SB_RECOVERED_MESSAGE_PUMP_H

#include <stdint.h>

/* Derived from BLIZZARD.EXE function 1008:e412.
 * This is a native test seam, not a Win16 compatibility layer. */
typedef struct SBRecoveredMessage {
    uint16_t hwnd;
    uint16_t message;
    uint16_t wparam;
    uint32_t lparam;
    uint32_t time;
    int16_t x;
    int16_t y;
} SBRecoveredMessage;

typedef struct SBRecoveredMessageAPI {
    int (*peek_message)(void *context, SBRecoveredMessage *message,
                        uint16_t min_message, uint16_t max_message);
    void (*translate_message)(void *context, const SBRecoveredMessage *message);
    void (*dispatch_message)(void *context, const SBRecoveredMessage *message);
    void *context;
} SBRecoveredMessageAPI;

/* flags are the observed low-byte control bits from 1008:e412:
 * 0x02 mouse-button batches, 0x04 keyboard/other batches,
 * 0x08 WM_KEYDOWN-like range, 0x10 WM_KEYUP-like range,
 * 0x40 WM_CHAR-like range. The final 0x0200 batch is unconditional. */
void sb_recovered_message_pump(const SBRecoveredMessageAPI *api, uint16_t flags);

#endif

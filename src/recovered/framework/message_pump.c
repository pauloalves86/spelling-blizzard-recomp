#include "message_pump.h"

static void drain_range(const SBRecoveredMessageAPI *api,
                        uint16_t min_message, uint16_t max_message)
{
    SBRecoveredMessage message;

    while (api->peek_message(api->context, &message,
                             min_message, max_message) != 0) {
        api->translate_message(api->context, &message);
        api->dispatch_message(api->context, &message);
    }
}

void sb_recovered_message_pump(const SBRecoveredMessageAPI *api, uint16_t flags)
{
    /* The message ranges and ordering below are recovered from the repeated
     * PEEKMESSAGE/TRANSLATEMESSAGE/DISPATCHMESSAGE blocks in 1008:e412.
     * The two unfiltered loops in the 0x04 block remain represented as
     * [0, 0]; their exact Win16 filter arguments need assembly validation. */
    if ((flags & 0x0002u) != 0) {
        static const uint16_t ranges[][2] = {
            {0x0201u, 0x0201u}, {0x0207u, 0x0207u},
            {0x0204u, 0x0204u}, {0x0203u, 0x0203u},
            {0x0209u, 0x0209u}, {0x0206u, 0x0206u}
        };
        for (unsigned i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
            drain_range(api, ranges[i][0], ranges[i][1]);
    }

    if ((flags & 0x0004u) != 0) {
        drain_range(api, 0x0202u, 0x0202u);
        drain_range(api, 0x0000u, 0x0000u);
        drain_range(api, 0x0000u, 0x0000u);
    }

    if ((flags & 0x0008u) != 0)
        drain_range(api, 0x0100u, 0x0100u);
    if ((flags & 0x0010u) != 0)
        drain_range(api, 0x0101u, 0x0101u);
    if ((flags & 0x0040u) != 0)
        drain_range(api, 0x000fu, 0x000fu);

    drain_range(api, 0x0200u, 0x0200u);
}

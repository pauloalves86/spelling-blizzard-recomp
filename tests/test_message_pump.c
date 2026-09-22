#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "message_pump.h"

typedef struct TestContext {
    uint16_t queued[16];
    size_t count;
    unsigned char consumed[16];
    uint16_t seen[16];
    size_t seen_count;
    unsigned translated;
    unsigned dispatched;
} TestContext;

static int peek(void *opaque, SBRecoveredMessage *out,
                uint16_t min_message, uint16_t max_message)
{
    TestContext *ctx = opaque;
    for (size_t i = 0; i < ctx->count; ++i) {
        uint16_t m = ctx->queued[i];
        if (!ctx->consumed[i] && m >= min_message && m <= max_message) {
            ctx->consumed[i] = 1;
            out->message = m;
            return 1;
        }
    }
    return 0;
}

static void translate(void *opaque, const SBRecoveredMessage *message)
{
    TestContext *ctx = opaque;
    (void)message;
    ++ctx->translated;
}

static void dispatch(void *opaque, const SBRecoveredMessage *message)
{
    TestContext *ctx = opaque;
    assert(ctx->seen_count < sizeof(ctx->seen) / sizeof(ctx->seen[0]));
    ctx->seen[ctx->seen_count++] = message->message;
    ++ctx->dispatched;
}

int main(void)
{
    TestContext ctx = {
        .queued = {0x0201u, 0x0207u, 0x0100u, 0x0200u},
        .count = 4
    };
    const SBRecoveredMessageAPI api = {
        .peek_message = peek,
        .translate_message = translate,
        .dispatch_message = dispatch,
        .context = &ctx
    };

    sb_recovered_message_pump(&api, 0x0002u | 0x0008u);
    assert(ctx.dispatched == 4);
    assert(ctx.translated == 4);
    assert(ctx.seen[0] == 0x0201u);
    assert(ctx.seen[1] == 0x0207u);
    assert(ctx.seen[2] == 0x0100u);
    assert(ctx.seen[3] == 0x0200u);
    return 0;
}

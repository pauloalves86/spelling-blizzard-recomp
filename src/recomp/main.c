/* Spelling Blizzard — statically recompiled original executable.
 *
 * Loads original/BLIZZARD.EXE into emulated Win16 memory, then runs the
 * recompiled entry point (1000:001a, the C runtime start-up which calls
 * WinMain). All Windows API calls land in the host Win16 layer (src/recomp/
 * kernel.c, user.c, gdi.c, mmsystem.c) implemented on SDL2.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "hostfs.h"
#include "loader.h"
#include "win16.h"

static void usage(void)
{
    fprintf(stderr,
            "usage: sb_recomp [options]\n"
            "  --game-dir DIR    directory with BLIZZARD.EXE and SB_CDRES.DLL (default: original)\n"
            "  --data-dir DIR    writable directory for INI and player files (default: run)\n"
            "  --scale N         window scale (default 2)\n"
            "  --no-audio        disable audio output\n"
            "  --headless        no window/audio device (SDL dummy drivers)\n"
            "  --quit-after MS   exit after MS milliseconds of game time\n"
            "  --snapshot FILE   write the final screen as PPM\n"
            "  --script FILE     scripted input events (see docs/recomp.md)\n"
            "  --trace-api       log API calls\n");
}

int main(int argc, char **argv)
{
    const char *game_dir = "original";
    const char *data_dir = "run";
    char exe[1024];
    Win16Options opt;
    int i;

    memset(&opt, 0, sizeof(opt));
    opt.scale = 2;
    opt.audio = 1;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--game-dir") && i + 1 < argc) game_dir = argv[++i];
        else if (!strcmp(argv[i], "--data-dir") && i + 1 < argc) data_dir = argv[++i];
        else if (!strcmp(argv[i], "--scale") && i + 1 < argc) opt.scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--no-audio")) opt.audio = 0;
        else if (!strcmp(argv[i], "--headless")) opt.headless = 1;
        else if (!strcmp(argv[i], "--quit-after") && i + 1 < argc) opt.max_ms = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--snapshot") && i + 1 < argc) opt.snapshot = argv[++i];
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) opt.script = argv[++i];
        else if (!strcmp(argv[i], "--trace-api")) g_api_trace = 1;
        else { usage(); return 2; }
    }

    hostfs_init(game_dir, data_dir);
    recomp_init_memory();
    snprintf(exe, sizeof(exe), "%s/BLIZZARD.EXE", game_dir);
    if (loader_load_exe(exe) != 0) {
        fprintf(stderr, "cannot load %s\n", exe);
        return 1;
    }
    if (win16_init(&opt) != 0) {
        return 1;
    }
    loader_setup_task("", 1 /* SW_SHOWNORMAL */);
    /* The loader "calls" the entry point: push a sentinel far return address. */
    PUSH16(THUNK_SEL);
    PUSH16(0xFFFF);
    recomp_start();
    fprintf(stderr, "[recomp] entry point returned\n");
    win16_shutdown();
    return 0;
}

/* Spelling Blizzard — the original game, recompiled.
 *
 * Loads BLIZZARD.EXE into emulated Win16 memory, then runs the recompiled
 * entry point (1000:001a, the C runtime start-up which calls WinMain). All
 * Windows API calls land in the host Win16 layer (src/recomp/kernel.c,
 * user.c, gdi.c, mmsystem.c) implemented on SDL2.
 *
 * As the player's program it keeps everything in one data folder (player.h):
 *   SpellingBlizzard --prepare --game=PATH   check your copy and keep it
 *   SpellingBlizzard                         play
 * For development, --game-dir/--data-dir play the files in place.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "api.h"
#include "hostfs.h"
#include "loader.h"
#include "player.h"
#include "win16.h"

static void usage(void)
{
    fprintf(stderr,
            "usage: SpellingBlizzard [options]\n"
            "\n"
            "  --prepare         check your copy of the game and keep it, then stop (no window)\n"
            "  --game PATH       your copy: the folder with BLIZZARD.EXE and SB_CDRES.DLL, one of\n"
            "                    those files, or the CD image (.iso, or .cue with its .bin)\n"
            "  --data DIR        where your copy, saves and logs go (default: $RECOMPLAUNCHER_DATA,\n"
            "                    else ~/.local/share/spelling-blizzard-recomp)\n"
            "  --scale N         window scale (default 2)\n"
            "  --fullscreen      start full screen\n"
            "  --no-audio        disable audio output\n"
            "\n"
            "development:\n"
            "  --game-dir DIR    play BLIZZARD.EXE and SB_CDRES.DLL in DIR in place (default: original)\n"
            "  --data-dir DIR    writable directory for INI and player files (default: run)\n"
            "  --headless        no window/audio device (SDL dummy drivers)\n"
            "  --quit-after MS   exit after MS milliseconds of game time\n"
            "  --snapshot FILE   write the final screen as PPM\n"
            "  --script FILE     scripted input events (see docs/recomp.md)\n"
            "  --trace-api       log API calls\n"
            "  --self-test       check that compiling in memory works here, then stop\n");
}

/* "--name VALUE" or "--name=VALUE"; advances *i past a separate value. */
static const char *opt_value(int argc, char **argv, int *i, const char *name)
{
    size_t n = strlen(name);
    if (strncmp(argv[*i], name, n)) return NULL;
    if (argv[*i][n] == '=') return argv[*i] + n + 1;
    if (argv[*i][n] == 0 && *i + 1 < argc) return argv[++*i];
    return NULL;
}

int main(int argc, char **argv)
{
    const char *game_dir = NULL, *data_dir = NULL, *data = NULL, *copy = NULL, *v;
    static char data_buf[1024], game_buf[1100], saves_buf[1100];
    int prepare = 0, self_test = 0, player = 0;
    struct stat st;
    char exe[1200];
    Win16Options opt;
    int i;

    memset(&opt, 0, sizeof(opt));
    opt.scale = 2;
    opt.audio = 1;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--prepare")) prepare = 1;
        else if (!strcmp(argv[i], "--self-test")) self_test = 1;
        else if ((v = opt_value(argc, argv, &i, "--game-dir"))) game_dir = v;
        else if ((v = opt_value(argc, argv, &i, "--game"))) copy = v;
        else if ((v = opt_value(argc, argv, &i, "--data-dir"))) data_dir = v;
        else if ((v = opt_value(argc, argv, &i, "--data"))) data = v;
        else if (!strcmp(argv[i], "--fullscreen")) opt.fullscreen = 1;
        else if ((v = opt_value(argc, argv, &i, "--scale"))) opt.scale = atoi(v);
        else if (!strcmp(argv[i], "--no-audio")) opt.audio = 0;
        else if (!strcmp(argv[i], "--headless")) opt.headless = 1;
        else if ((v = opt_value(argc, argv, &i, "--quit-after"))) opt.max_ms = (uint32_t)strtoul(v, NULL, 10);
        else if ((v = opt_value(argc, argv, &i, "--snapshot"))) opt.snapshot = v;
        else if ((v = opt_value(argc, argv, &i, "--script"))) opt.script = v;
        else if (!strcmp(argv[i], "--trace-api")) g_api_trace = 1;
        else { usage(); return 2; }
    }
    if (self_test) return recomp_self_test() ? 1 : 0;
    if (!data) {
        player_default_data_dir(data_buf, sizeof data_buf);
        data = data_buf;
    }
    if (prepare) return player_prepare(data, copy);

    if (!game_dir && !data_dir) {
        /* the player's program: the copy kept in the data folder */
        if (copy) {
            int rc = player_install(data, copy);
            if (rc) return rc;
        }
        if (player_has_copy(data)) {
            snprintf(game_buf, sizeof game_buf, "%s/game", data);
            snprintf(saves_buf, sizeof saves_buf, "%s/saves", data);
            make_dirs(saves_buf);
            game_dir = game_buf;
            data_dir = saves_buf;
            player = 1;
        } else if (stat("original/BLIZZARD.EXE", &st) != 0) {
            fprintf(stderr, "Spelling Blizzard isn't set up yet. Run: %s --prepare --game=/path/to/your/copy\n",
                    argv[0]);
            return EXIT_NOT_SET_UP;
        }
    }
    if (!game_dir) game_dir = "original";
    if (!data_dir) data_dir = "run";
    if (player) {
        player_start_log(data);
        const char *mode = getenv("RECOMPLAUNCHER_MODE");
        if (mode && !strcmp(mode, "bigpicture")) opt.fullscreen = 1;
    }

    hostfs_init(game_dir, data_dir);
    recomp_init_memory();
    if (hostfs_find_game_file("BLIZZARD.EXE", exe, sizeof(exe)) != 0) {
        snprintf(exe, sizeof(exe), "%s/BLIZZARD.EXE", game_dir);
    }
    char err[1024];
    if (recomp_load_program(exe, err, sizeof err) != 0) {
        fprintf(stderr, "Can't recompile %s: %s\n", exe, err);
        return EXIT_RECOMPILE;
    }
    if (loader_load_exe(exe) != 0) {
        fprintf(stderr, "cannot load %s\n", exe);
        return EXIT_BAD_COPY;
    }
    if (win16_init(&opt) != 0) {
        return 1;
    }
    loader_setup_task("", 1 /* SW_SHOWNORMAL */);
    /* The loader "calls" the entry point: push a sentinel far return address. */
    PUSH16(THUNK_SEL);
    PUSH16(0xFFFF);
    recomp_program.start();
    fprintf(stderr, "[recomp] entry point returned\n");
    win16_shutdown();
    return 0;
}

/* Running as the player's program: one data folder holds the player's copy
 * of the game (game/), saves (saves/) and logs (logs/). --prepare checks and
 * keeps the copy; playing finds it there. Exit codes are what launchers show
 * (recomplauncher.json). */
#ifndef SB_RECOMP_PLAYER_H
#define SB_RECOMP_PLAYER_H

#include <stddef.h>

enum {
    EXIT_NOT_SET_UP = 64,    /* no copy of the game kept yet */
    EXIT_BAD_COPY = 65,      /* the picked copy is missing, wrong or unreadable */
    EXIT_RECOMPILE = 67,     /* recompiling the copy failed */
    EXIT_UNSUPPORTED = 70,   /* the game reached something the recomp can't do yet */
};

/* The data folder: $RECOMPLAUNCHER_DATA, else the platform's per-user data
 * folder (~/.local/share/spelling-blizzard-recomp). */
void player_default_data_dir(char *out, size_t cap);
/* Reads the player's copy (folder, one of its files, or a CD image), checks it
 * and keeps it in DATA/game. Returns 0, or an EXIT_* code after printing why. */
int player_install(const char *data, const char *copy);
/* --prepare: install (or, without a copy, re-check the kept one), then
 * recompile it once to be sure it plays. Prints "progress <0..1> <text>". */
int player_prepare(const char *data, const char *copy);
/* Whether DATA/game holds both files. */
int player_has_copy(const char *data);
/* Copies everything written to stderr into DATA/logs/SpellingBlizzard-*.log
 * (keeping the last five), and turns a crash into EXIT_UNSUPPORTED. */
void player_start_log(const char *data);
int make_dirs(const char *path);

#endif

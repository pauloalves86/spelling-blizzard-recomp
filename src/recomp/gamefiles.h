/* Finding the player's game files (BLIZZARD.EXE and SB_CDRES.DLL) in what
 * they picked: a folder (searched a few levels deep, any letter case), a file
 * in it, a CD image (.iso, or a raw .bin/.img with its .cue), and checking
 * that they are the version the recompiler knows. */
#ifndef SB_GAMEFILES_H
#define SB_GAMEFILES_H

#include <stddef.h>
#include <stdint.h>

#define GAME_EXE_NAME "BLIZZARD.EXE"
#define GAME_DLL_NAME "SB_CDRES.DLL"
#define GAME_EXE_SHA256 "ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2"
#define GAME_DLL_SHA256 "1c6c36bde51b8c9ad3b3a0eaba0283d0468d81326b3dbe4dbfbf7305863b70e1"

typedef struct GameFiles {
    uint8_t *exe, *dll;
    size_t exe_len, dll_len;
} GameFiles;

/* Reads both files from path. Returns 0, or -1 with a message for the player. */
int gamefiles_read(const char *path, GameFiles *out, char *err, size_t errcap);
/* Checks both files against the known SHA-256. Returns 0, or -1 with a message. */
int gamefiles_check(const GameFiles *g, char *err, size_t errcap);
/* Writes both files into dir (which must exist) under their usual names. */
int gamefiles_write(const GameFiles *g, const char *dir, char *err, size_t errcap);
void gamefiles_free(GameFiles *g);

#endif

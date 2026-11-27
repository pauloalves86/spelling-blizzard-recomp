#include "hostfs.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static char g_game_dir[512];
static char g_data_dir[512];

void hostfs_init(const char *game_dir, const char *data_dir)
{
    snprintf(g_game_dir, sizeof(g_game_dir), "%s", game_dir);
    snprintf(g_data_dir, sizeof(g_data_dir), "%s", data_dir);
    mkdir(g_data_dir, 0755);
}

const char *hostfs_data_dir(void) { return g_data_dir; }

static const char *basename_dos(const char *p)
{
    const char *b = p;
    for (; *p; ++p) {
        if (*p == '\\' || *p == '/' || *p == ':') {
            b = p + 1;
        }
    }
    return b;
}

static int find_ci(const char *dir, const char *name, char *out, size_t cap)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) {
        return -1;
    }
    while ((e = readdir(d)) != NULL) {
        if (strcasecmp(e->d_name, name) == 0) {
            snprintf(out, cap, "%s/%s", dir, e->d_name);
            closedir(d);
            return 0;
        }
    }
    closedir(d);
    return -1;
}

/* Split a DOS path into normalized components relative to the game's
 * install directory (C:\SB). Trailing dots are dropped like DOS does. */
static int dos_components(const char *dos, char comp[][16], int maxc)
{
    char buf[260];
    char *p, *tok;
    int n = 0;
    snprintf(buf, sizeof(buf), "%s", dos);
    p = buf;
    if (isalpha((unsigned char)p[0]) && p[1] == ':') {
        p += 2;
    }
    for (tok = strtok(p, "\\/"); tok && n < maxc; tok = strtok(NULL, "\\/")) {
        size_t L;
        int i;
        if (!strcmp(tok, ".") || !*tok) continue;
        if (!strcmp(tok, "..")) { if (n) n--; continue; }
        L = strlen(tok);
        while (L > 0 && tok[L - 1] == '.') tok[--L] = 0;
        if (!L) continue;
        for (i = 0; tok[i] && i < 15; ++i) comp[n][i] = (char)toupper((unsigned char)tok[i]);
        comp[n][i] = 0;
        n++;
    }
    /* drop the install-directory prefix (C:\SB, C:\WINDOWS) */
    if (n > 1 && (!strcmp(comp[0], "SB") || !strcmp(comp[0], "WINDOWS"))) {
        memmove(comp[0], comp[1], (size_t)(n - 1) * 16);
        n--;
    }
    return n;
}

/* Resolve components under the data dir; dirs matched case-insensitively. */
static int data_path(const char *dos, char *out, size_t cap, int create_dirs)
{
    char comp[16][16];
    int n = dos_components(dos, comp, 16), i;
    char cur[1024];
    snprintf(cur, sizeof(cur), "%s", g_data_dir);
    for (i = 0; i < n; ++i) {
        char next[1024];
        if (find_ci(cur, comp[i], next, sizeof(next)) != 0) {
            snprintf(next, sizeof(next), "%s/%s", cur, comp[i]);
            if (i < n - 1 && create_dirs) {
                mkdir(next, 0755);
            }
        }
        snprintf(cur, sizeof(cur), "%s", next);
    }
    snprintf(out, cap, "%s", cur);
    return n;
}

int hostfs_resolve_existing(const char *dos_path, char *out, size_t cap)
{
    struct stat st;
    const char *name = basename_dos(dos_path);
    char tmp[1024];
    data_path(dos_path, tmp, sizeof(tmp), 0);
    if (stat(tmp, &st) == 0) {
        snprintf(out, cap, "%s", tmp);
        return 0;
    }
    if (!*name) {
        return -1;
    }
    /* read-only game files (EXE, CD resource DLL) by file name */
    return find_ci(g_game_dir, name, out, cap);
}

int hostfs_find_game_file(const char *name, char *out, size_t cap)
{
    return find_ci(g_game_dir, name, out, cap);
}

void hostfs_resolve_create(const char *dos_path, char *out, size_t cap)
{
    data_path(dos_path, out, cap, 1);
}

int hostfs_resolve_dir(const char *dos_dir, char *out, size_t cap)
{
    struct stat st;
    data_path(dos_dir, out, cap, 0);
    return (stat(out, &st) == 0 && S_ISDIR(st.st_mode)) ? 0 : -1;
}

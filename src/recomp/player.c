/* Player mode: see player.h. */
#include "player.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "gamefiles.h"
#include "runtime.h"

int make_dirs(const char *path)
{
    char p[1024];
    snprintf(p, sizeof p, "%s", path);
    for (char *s = p + 1; *s; s++) {
        if (*s != '/') continue;
        *s = 0;
        if (mkdir(p, 0755) && errno != EEXIST) return -1;
        *s = '/';
    }
    return mkdir(p, 0755) && errno != EEXIST ? -1 : 0;
}

void player_default_data_dir(char *out, size_t cap)
{
    const char *rl = getenv("RECOMPLAUNCHER_DATA"), *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    if (rl && *rl) snprintf(out, cap, "%s", rl);
#ifdef __APPLE__
    else if (home && *home) snprintf(out, cap, "%s/Library/Application Support/spelling-blizzard-recomp", home);
#endif
    else if (xdg && *xdg) snprintf(out, cap, "%s/spelling-blizzard-recomp", xdg);
    else if (home && *home) snprintf(out, cap, "%s/.local/share/spelling-blizzard-recomp", home);
    else snprintf(out, cap, "spelling-blizzard-recomp");
}

static void progress(double f, const char *text)
{
    printf("progress %.2f %s\n", f, text);
    fflush(stdout);
}

int player_has_copy(const char *data)
{
    char p[1100];
    struct stat st;
    snprintf(p, sizeof p, "%s/game/" GAME_EXE_NAME, data);
    if (stat(p, &st)) return 0;
    snprintf(p, sizeof p, "%s/game/" GAME_DLL_NAME, data);
    return stat(p, &st) == 0;
}

int player_install(const char *data, const char *copy)
{
    GameFiles g;
    char err[1200], dir[1100];
    if (gamefiles_read(copy, &g, err, sizeof err) || gamefiles_check(&g, err, sizeof err)) {
        gamefiles_free(&g);
        fprintf(stderr, "%s\n", err);
        return EXIT_BAD_COPY;
    }
    snprintf(dir, sizeof dir, "%s/game", data);
    if (make_dirs(dir)) {
        gamefiles_free(&g);
        fprintf(stderr, "Can't create %s: %s.\n", dir, strerror(errno));
        return EXIT_BAD_COPY;
    }
    int rc = gamefiles_write(&g, dir, err, sizeof err);
    gamefiles_free(&g);
    if (rc) {
        fprintf(stderr, "%s\n", err);
        return EXIT_BAD_COPY;
    }
    return 0;
}

int player_prepare(const char *data, const char *copy)
{
    char dir[1100], exe[1200], err[1024];
    progress(0.05, "Checking your copy");
    if (copy) {
        int rc = player_install(data, copy);
        if (rc) return rc;
    } else {
        GameFiles g;
        snprintf(dir, sizeof dir, "%s/game", data);
        if (!player_has_copy(data)) {
            fprintf(stderr, "Pick your copy of the game: the folder with BLIZZARD.EXE and SB_CDRES.DLL, or your CD "
                            "image (.iso, or .cue with its .bin).\n");
            return EXIT_BAD_COPY;
        }
        int bad = gamefiles_read(dir, &g, err, sizeof err) || gamefiles_check(&g, err, sizeof err);
        gamefiles_free(&g);
        if (bad) {
            fprintf(stderr, "%s\n", err);
            return EXIT_BAD_COPY;
        }
    }
    progress(0.6, "Recompiling the game");
    snprintf(exe, sizeof exe, "%s/game/" GAME_EXE_NAME, data);
    if (recomp_load_program(exe, err, sizeof err)) {
        fprintf(stderr, "Recompiling %s failed: %s\n", exe, err);
        return EXIT_RECOMPILE;
    }
    snprintf(dir, sizeof dir, "%s/saves", data);
    make_dirs(dir);
    snprintf(dir, sizeof dir, "%s/logs", data);
    make_dirs(dir);
    progress(1, "Ready to play");
    return 0;
}

/* ------------------------------------------------------------ logs */

static int g_tty = -1, g_log = -1, g_pipe = -1;
static pthread_t g_tee;
static volatile sig_atomic_t g_tee_done;

static void *tee_main(void *arg)
{
    (void)arg;
    char buf[4096];
    ssize_t n;
    while ((n = read(g_pipe, buf, sizeof buf)) > 0) {
        if (write(g_tty, buf, (size_t)n) < 0) { /* the terminal went away: keep logging */ }
        if (write(g_log, buf, (size_t)n) < 0) break;
    }
    g_tee_done = 1;
    return NULL;
}

static void write_both(const char *s)
{
    size_t n = strlen(s);
    if (g_tty >= 0 && write(g_tty, s, n) < 0) { /* nothing to do */ }
    if (g_log >= 0 && write(g_log, s, n) < 0) { /* nothing to do */ }
}

/* At exit: let the tee thread write out what is still in the pipe. */
static void finish_log(void)
{
    fflush(stderr);
    int devnull = open("/dev/null", O_WRONLY);  /* closing fd 2 ends the pipe */
    if (devnull >= 0) {
        dup2(devnull, 2);
        close(devnull);
    } else {
        close(2);
    }
    pthread_join(g_tee, NULL);
}

static void on_crash(int sig)
{
    /* End the pipe and give the tee thread up to a second to copy what the
     * game wrote just before (recomp_fatal's registers and backtrace). */
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) dup2(devnull, 2);
    for (int i = 0; i < 100 && g_pipe >= 0 && !g_tee_done; i++) {
        struct timespec ts = {0, 10 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    char msg[256];
    snprintf(msg, sizeof msg,
             "\nSpelling Blizzard stopped on something the recomp can't do yet (signal %d). "
             "Please report it with this log.\n",
             sig);
    write_both(msg);
    _exit(EXIT_UNSUPPORTED);
}

static int cmp_names(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

void player_start_log(const char *data)
{
    char dir[1100], path[1300];
    snprintf(dir, sizeof dir, "%s/logs", data);
    make_dirs(dir);

    /* keep the newest four, plus the one started now */
    DIR *d = opendir(dir);
    if (d) {
        char *names[512];
        int n = 0;
        struct dirent *de;
        while ((de = readdir(d)) && n < 512)
            if (!strncmp(de->d_name, "SpellingBlizzard-", 17) && strstr(de->d_name, ".log")) names[n++] = strdup(de->d_name);
        closedir(d);
        qsort(names, (size_t)n, sizeof names[0], cmp_names);
        for (int i = 0; i < n; i++) {
            if (i < n - 4 && names[i]) {
                snprintf(path, sizeof path, "%s/%s", dir, names[i]);
                remove(path);
            }
            free(names[i]);
        }
    }

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    snprintf(path, sizeof path, "%s/SpellingBlizzard-%04d%02d%02d-%02d%02d%02d.log", dir, tm.tm_year + 1900,
             tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    FILE *fp = fopen(path, "w");
    int p[2];
    if (fp && pipe(p) == 0) {
        fflush(stderr);
        g_log = dup(fileno(fp));
        g_tty = dup(2);
        g_pipe = p[0];
        dup2(p[1], 2);
        close(p[1]);
        setvbuf(stderr, NULL, _IONBF, 0);
        if (pthread_create(&g_tee, NULL, tee_main, NULL) == 0) atexit(finish_log);
    }
    if (fp) fclose(fp);

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_crash;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

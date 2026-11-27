/* sb_gen: writes the recompiled C for BLIZZARD.EXE into a directory.
 *
 *   sb_gen OUT_DIR --exe PATH [--imports DIR [--update-imports]] [--functions FILE]
 *
 * The import tables the host runtime is compiled against are checked in under
 * src/recomp/imports/ (they list API names and ordinals, no game code), so the
 * runtime builds without the game files. With --imports, sb_gen checks that
 * they match the EXE; --update-imports rewrites them.
 */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "gen.h"

static char *read_file(const char *path, size_t *len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    char *buf = NULL;
    size_t n = 0, cap = 0;
    for (;;) {
        if (n == cap) {
            cap = cap ? cap * 2 : 1 << 16;
            char *nb = realloc(buf, cap + 1);
            if (!nb) {
                free(buf);
                fclose(fp);
                return NULL;
            }
            buf = nb;
        }
        size_t r = fread(buf + n, 1, cap - n, fp);
        n += r;
        if (r == 0) break;
    }
    fclose(fp);
    buf[n] = 0;
    *len = n;
    return buf;
}

static int write_file(const char *dir, const char *name, const char *data, size_t len) {
    char path[4096];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    size_t curlen;
    char *cur = read_file(path, &curlen);
    int same = cur && curlen == len && !memcmp(cur, data, len);
    free(cur);
    if (same) return 0;  /* unchanged: keep the timestamp, so nothing recompiles */
    FILE *fp = fopen(path, "wb");
    if (!fp || fwrite(data, 1, len, fp) != len || fclose(fp)) {
        fprintf(stderr, "sb_gen: can't write %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

/* Compares one checked-in import table with the EXE's; rewrites it when asked. */
static int check_import_table(const char *dir, const GenFile *f, int update) {
    char path[4096];
    size_t len = 0;
    snprintf(path, sizeof path, "%s/%s", dir, f->name);
    char *cur = read_file(path, &len);
    int same = cur && len == f->len && !memcmp(cur, f->data, len);
    free(cur);
    if (same) return 0;
    if (!update) {
        fprintf(stderr, "sb_gen: %s does not match the EXE's imports; rerun with --update-imports\n", path);
        return -1;
    }
    if (write_file(dir, f->name, f->data, f->len)) return -1;
    printf("updated %s\n", path);
    return 0;
}

/* Removes generated files a previous run wrote that this one didn't. */
static void remove_stale(const char *dir, const GenOutput *out) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        const char *n = de->d_name;
        size_t len = strlen(n);
        if (strncmp(n, "gen_", 4) || len < 3 || strcmp(n + len - 2, ".c")) continue;
        int keep = 0;
        for (int k = 0; k < out->nfiles && !keep; k++) keep = !strcmp(out->files[k].name, n);
        if (!keep) {
            char path[4096];
            snprintf(path, sizeof path, "%s/%s", dir, n);
            remove(path);
        }
    }
    closedir(d);
}

static void usage(void) {
    fprintf(stderr, "usage: sb_gen OUT_DIR --exe PATH [--imports DIR [--update-imports]] [--functions FILE]\n");
    exit(2);
}

int main(int argc, char **argv) {
    const char *out_dir = NULL, *exe_path = NULL, *imports = NULL, *functions = NULL;
    int update = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--exe") && i + 1 < argc) exe_path = argv[++i];
        else if (!strcmp(argv[i], "--imports") && i + 1 < argc) imports = argv[++i];
        else if (!strcmp(argv[i], "--functions") && i + 1 < argc) functions = argv[++i];
        else if (!strcmp(argv[i], "--update-imports")) update = 1;
        else if (argv[i][0] != '-' && !out_dir) out_dir = argv[i];
        else usage();
    }
    if (!out_dir || !exe_path) usage();

    size_t len, flen;
    char *exe = read_file(exe_path, &len);
    if (!exe) {
        fprintf(stderr, "sb_gen: can't read %s: %s\n", exe_path, strerror(errno));
        return 1;
    }
    char *ftxt = NULL;
    if (functions && !(ftxt = read_file(functions, &flen))) {
        fprintf(stderr, "sb_gen: can't read %s: %s\n", functions, strerror(errno));
        free(exe);
        return 1;
    }

    GenOutput out;
    char err[512];
    int failed = gen_recompile((const uint8_t *)exe, len, ftxt, &out, err, sizeof err);
    free(exe);
    free(ftxt);
    if (failed) {
        fprintf(stderr, "sb_gen: %s\n", err);
        return 1;
    }

    if (mkdir(out_dir, 0777) && errno != EEXIST) {
        fprintf(stderr, "sb_gen: can't create %s: %s\n", out_dir, strerror(errno));
        gen_output_free(&out);
        return 1;
    }
    int rc = 0;
    for (int k = 0; k < out.nfiles && !rc; k++)
        rc = write_file(out_dir, out.files[k].name, out.files[k].data, out.files[k].len);
    if (!rc) remove_stale(out_dir, &out);
    if (!rc && imports)
        rc = check_import_table(imports, &out.import_list, update) | check_import_table(imports, &out.api_decls, update);
    if (!rc)
        printf("%d functions, %d imports called directly, %d unresolved call targets\n", out.nfuncs,
               out.nimports_called, out.nunresolved);
    gen_output_free(&out);
    return rc ? 1 : 0;
}

/* Finding and checking the player's game files: see gamefiles.h. */
#include "gamefiles.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sha256.h"

#define MAX_GAME_FILE (64u << 20)  /* far above either file; guards against nonsense sizes */
#define SEARCH_DEPTH 3

static void set_err(char *err, size_t cap, const char *fmt, const char *a, const char *b)
{
    snprintf(err, cap, fmt, a, b);
}

static int is_dir(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static int ieq(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) return 0;
    return *a == *b;
}

static int ends_with_ci(const char *s, const char *suffix)
{
    size_t n = strlen(s), m = strlen(suffix);
    return n >= m && ieq(s + n - m, suffix);
}

static uint8_t *read_whole(const char *path, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    uint8_t *buf = NULL;
    size_t n = 0, cap = 0, r;
    do {
        if (n == cap) {
            if (cap >= MAX_GAME_FILE) {
                free(buf);
                fclose(fp);
                return NULL;
            }
            cap = cap ? cap * 2 : 1 << 16;
            uint8_t *nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                fclose(fp);
                return NULL;
            }
            buf = nb;
        }
        r = fread(buf + n, 1, cap - n, fp);
        n += r;
    } while (r);
    fclose(fp);
    *len = n;
    return buf;
}

/* ------------------------------------------------------------ folders */

/* Breadth-first, so the shallowest match wins; any letter case. */
static int find_in_dir(const char *dir, const char *name, int depth, char *out, size_t cap)
{
    DIR *d = opendir(dir);
    if (!d) return -1;
    struct dirent *de;
    char sub[64][1024];
    int nsub = 0, found = -1;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char p[1024];
        snprintf(p, sizeof p, "%s/%s", dir, de->d_name);
        if (ieq(de->d_name, name) && !is_dir(p)) {
            snprintf(out, cap, "%s", p);
            found = 0;
            break;
        }
        if (depth > 0 && nsub < 64 && is_dir(p)) snprintf(sub[nsub++], sizeof sub[0], "%s", p);
    }
    closedir(d);
    for (int i = 0; i < nsub && found; i++) found = find_in_dir(sub[i], name, depth - 1, out, cap);
    return found;
}

static int read_from_folder(const char *dir, GameFiles *g, char *err, size_t errcap)
{
    const char *names[2] = {GAME_EXE_NAME, GAME_DLL_NAME};
    uint8_t **bufs[2] = {&g->exe, &g->dll};
    size_t *lens[2] = {&g->exe_len, &g->dll_len};
    for (int k = 0; k < 2; k++) {
        char p[1024];
        if (find_in_dir(dir, names[k], SEARCH_DEPTH, p, sizeof p)) {
            set_err(err, errcap, "%s isn't in %s. Pick the folder with BLIZZARD.EXE and SB_CDRES.DLL, or your CD image.",
                    names[k], dir);
            return -1;
        }
        if (!(*bufs[k] = read_whole(p, lens[k]))) {
            set_err(err, errcap, "Can't read %s: %s.", p, strerror(errno));
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------ CD images */

typedef struct Image {
    FILE *fp;
    unsigned sector;     /* bytes per sector in the file: 2048, or 2352 for raw */
    unsigned data_off;   /* where the 2048 data bytes start in a sector */
    uint64_t nsectors;
} Image;

static int read_sector(Image *im, uint32_t lba, uint8_t out[2048])
{
    if (lba >= im->nsectors) return -1;
    if (fseek(im->fp, (long)((uint64_t)lba * im->sector + im->data_off), SEEK_SET)) return -1;
    return fread(out, 1, 2048, im->fp) == 2048 ? 0 : -1;
}

static uint32_t le32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

/* Sector layout: plain 2048-byte sectors (.iso), or raw 2352-byte ones
 * (.bin/.img: sync pattern, header, then data at 16 for mode 1 or 24 for
 * mode 2 form 1). */
static int image_open(Image *im, const char *path)
{
    static const uint8_t sync[12] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
    uint8_t head[16];
    memset(im, 0, sizeof *im);
    if (!(im->fp = fopen(path, "rb"))) return -1;
    if (fseek(im->fp, 0, SEEK_END)) return -1;
    long size = ftell(im->fp);
    if (size < 0 || fseek(im->fp, 0, SEEK_SET)) return -1;
    /* (a file too short for a header is simply not an image: no sectors) */
    if (fread(head, 1, 16, im->fp) == 16 && !memcmp(head, sync, 12)) {
        im->sector = 2352;
        im->data_off = head[15] == 2 ? 24 : 16;
    } else {
        im->sector = 2048;
        im->data_off = 0;
    }
    im->nsectors = (uint64_t)size / im->sector;
    return 0;
}

/* Directory record fields (ECMA-119 9.1). */
typedef struct DirRec {
    uint32_t lba, size;
    int is_dir;
    char name[64];
} DirRec;

static int parse_rec(const uint8_t *r, size_t avail, DirRec *out)
{
    unsigned len = r[0];
    if (len < 34 || len > avail) return -1;
    unsigned nl = r[32];
    if (33 + nl > len) return -1;
    out->lba = le32(r + 2);
    out->size = le32(r + 10);
    out->is_dir = (r[25] & 2) != 0;
    if (r[25] & 0x80) return -1;  /* multi-extent files: not needed here */
    if (nl == 1 && (r[33] == 0 || r[33] == 1)) {
        out->name[0] = 0;  /* "." and ".." */
        return 0;
    }
    size_t k = 0;
    for (unsigned i = 0; i < nl && r[33 + i] != ';' && k + 1 < sizeof out->name; i++) out->name[k++] = (char)r[33 + i];
    if (k && out->name[k - 1] == '.') k--;  /* "NAME." for names without extension */
    out->name[k] = 0;
    return 0;
}

/* Breadth-first search for a file by name below the root directory. */
static int iso_find(Image *im, const DirRec *root, const char *name, DirRec *found)
{
    DirRec queue[256];
    int depth[256], head = 0, tail = 0;
    queue[tail] = *root;
    depth[tail++] = 0;
    uint8_t sec[2048];
    while (head < tail) {
        DirRec dir = queue[head];
        int d = depth[head++];
        uint32_t nsec = (dir.size + 2047) / 2048;
        if (nsec > 64) nsec = 64;  /* no CD directory of this era is anywhere near this big */
        for (uint32_t s = 0; s < nsec; s++) {
            if (read_sector(im, dir.lba + s, sec)) return -1;
            size_t off = 0;
            while (off < 2048 && sec[off]) {
                DirRec r;
                if (parse_rec(sec + off, 2048 - off, &r)) break;
                off += sec[off];
                if (!r.name[0]) continue;
                if (!r.is_dir && ieq(r.name, name)) {
                    *found = r;
                    return 0;
                }
                if (r.is_dir && d < SEARCH_DEPTH && tail < 256) {
                    queue[tail] = r;
                    depth[tail++] = d + 1;
                }
            }
        }
    }
    return -1;
}

static uint8_t *iso_read_file(Image *im, const DirRec *r, size_t *len)
{
    if (r->size > MAX_GAME_FILE) return NULL;
    uint8_t *buf = malloc(r->size ? r->size : 1), sec[2048];
    if (!buf) return NULL;
    for (uint32_t done = 0, s = 0; done < r->size; s++) {
        if (read_sector(im, r->lba + s, sec)) {
            free(buf);
            return NULL;
        }
        uint32_t n = r->size - done < 2048 ? r->size - done : 2048;
        memcpy(buf + done, sec, n);
        done += n;
    }
    *len = r->size;
    return buf;
}

static int read_from_image(const char *path, GameFiles *g, char *err, size_t errcap)
{
    Image im;
    uint8_t sec[2048];
    DirRec root;
    int have_root = 0;
    if (image_open(&im, path)) {
        if (im.fp) fclose(im.fp);
        set_err(err, errcap, "Can't read %s: %s.", path, strerror(errno));
        return -1;
    }
    /* Volume descriptors from sector 16: the primary one (type 1) has the root. */
    for (uint32_t s = 16; s < 64 && !read_sector(&im, s, sec); s++) {
        if (memcmp(sec + 1, "CD001", 5)) break;
        if (sec[0] == 1 && !parse_rec(sec + 156, 34, &root) && root.is_dir) {
            have_root = 1;
            break;
        }
        if (sec[0] == 255) break;
    }
    if (!have_root) {
        fclose(im.fp);
        set_err(err, errcap, "%s isn't a CD image this can read (an ISO, or a raw BIN/IMG with its CUE).%s", path, "");
        return -1;
    }
    const char *names[2] = {GAME_EXE_NAME, GAME_DLL_NAME};
    uint8_t **bufs[2] = {&g->exe, &g->dll};
    size_t *lens[2] = {&g->exe_len, &g->dll_len};
    for (int k = 0; k < 2; k++) {
        DirRec r;
        if (iso_find(&im, &root, names[k], &r)) {
            fclose(im.fp);
            set_err(err, errcap, "%s isn't on this CD image (%s). Is it the Spelling Blizzard CD?", names[k], path);
            return -1;
        }
        if (!(*bufs[k] = iso_read_file(&im, &r, lens[k]))) {
            fclose(im.fp);
            set_err(err, errcap, "Can't read %s from %s: the image looks damaged or cut short.", names[k], path);
            return -1;
        }
    }
    fclose(im.fp);
    return 0;
}

/* The data file a .cue points to (its first FILE line), next to the .cue. */
static int cue_data_file(const char *cue, char *out, size_t cap)
{
    FILE *fp = fopen(cue, "r");
    if (!fp) return -1;
    char line[1024];
    int rc = -1;
    while (rc && fgets(line, sizeof line, fp)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (strncmp(p, "FILE", 4) || !isspace((unsigned char)p[4])) continue;
        p += 4;
        while (isspace((unsigned char)*p)) p++;
        char name[512];
        size_t k = 0;
        if (*p == '"') {
            for (p++; *p && *p != '"' && k + 1 < sizeof name; p++) name[k++] = *p;
        } else {
            for (; *p && !isspace((unsigned char)*p) && k + 1 < sizeof name; p++) name[k++] = *p;
        }
        name[k] = 0;
        if (!k) continue;
        const char *slash = strrchr(cue, '/');
        if (name[0] == '/' || !slash) snprintf(out, cap, "%s", name);
        else snprintf(out, cap, "%.*s/%s", (int)(slash - cue), cue, name);
        rc = 0;
    }
    fclose(fp);
    return rc;
}

/* ------------------------------------------------------------ public */

int gamefiles_read(const char *path, GameFiles *out, char *err, size_t errcap)
{
    memset(out, 0, sizeof *out);
    struct stat st;
    if (stat(path, &st)) {
        set_err(err, errcap, "%s doesn't exist.%s", path, "");
        return -1;
    }
    int rc;
    if (S_ISDIR(st.st_mode)) {
        rc = read_from_folder(path, out, err, errcap);
    } else if (ends_with_ci(path, ".cue")) {
        char data[1024];
        if (cue_data_file(path, data, sizeof data)) {
            set_err(err, errcap, "%s doesn't name a data file (no FILE line).%s", path, "");
            rc = -1;
        } else {
            rc = read_from_image(data, out, err, errcap);
        }
    } else if (ends_with_ci(path, ".chd")) {
        set_err(err, errcap, "CHD images aren't supported yet. Convert it first (chdman extractcd -i %s -o game.cue), "
                "or pick the folder with BLIZZARD.EXE and SB_CDRES.DLL.%s", path, "");
        rc = -1;
    } else if (ieq(strrchr(path, '/') ? strrchr(path, '/') + 1 : path, GAME_EXE_NAME) ||
               ieq(strrchr(path, '/') ? strrchr(path, '/') + 1 : path, GAME_DLL_NAME)) {
        /* one of the files itself: use its folder */
        char dir[1024];
        const char *slash = strrchr(path, '/');
        if (slash) snprintf(dir, sizeof dir, "%.*s", (int)(slash - path), path);
        else snprintf(dir, sizeof dir, ".");
        rc = read_from_folder(dir, out, err, errcap);
    } else {
        rc = read_from_image(path, out, err, errcap);
    }
    if (rc) gamefiles_free(out);
    return rc;
}

int gamefiles_check(const GameFiles *g, char *err, size_t errcap)
{
    char hex[65];
    sha256_hex(g->exe, g->exe_len, hex);
    if (strcmp(hex, GAME_EXE_SHA256)) {
        set_err(err, errcap, "This %s is a different version. The recomp needs Spelling Blizzard 1.10.CD for Windows.%s",
                GAME_EXE_NAME, "");
        return -1;
    }
    sha256_hex(g->dll, g->dll_len, hex);
    if (strcmp(hex, GAME_DLL_SHA256)) {
        set_err(err, errcap, "This %s is a different version. The recomp needs Spelling Blizzard 1.10.CD for Windows.%s",
                GAME_DLL_NAME, "");
        return -1;
    }
    return 0;
}

int gamefiles_write(const GameFiles *g, const char *dir, char *err, size_t errcap)
{
    const char *names[2] = {GAME_EXE_NAME, GAME_DLL_NAME};
    const uint8_t *bufs[2] = {g->exe, g->dll};
    size_t lens[2] = {g->exe_len, g->dll_len};
    for (int k = 0; k < 2; k++) {
        char p[1024], tmp[1100];
        snprintf(p, sizeof p, "%s/%s", dir, names[k]);
        snprintf(tmp, sizeof tmp, "%s.new", p);
        FILE *fp = fopen(tmp, "wb");
        int ok = fp && fwrite(bufs[k], 1, lens[k], fp) == lens[k];
        if (fp && fclose(fp)) ok = 0;
        if (!ok || rename(tmp, p)) {
            remove(tmp);
            set_err(err, errcap, "Can't write %s: %s.", p, strerror(errno));
            return -1;
        }
    }
    return 0;
}

void gamefiles_free(GameFiles *g)
{
    free(g->exe);
    free(g->dll);
    memset(g, 0, sizeof *g);
}

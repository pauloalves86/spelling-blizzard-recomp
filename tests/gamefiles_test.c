/* Tests for gamefiles.c and sha256.c with synthetic inputs (no game files):
 * a nested folder in mixed case, a plain ISO, and a raw BIN with its CUE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "gamefiles.h"
#include "sha256.h"

static int failures;
#define CHECK(c)                                                    \
    do {                                                            \
        if (!(c)) {                                                 \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c); \
            failures++;                                             \
        }                                                           \
    } while (0)

static uint8_t exe_data[3000], dll_data[5000];

static void put_le32_be32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)), p[7 - i] = (uint8_t)(v >> (8 * i));
}

/* One directory record; returns its length. */
static size_t dirrec(uint8_t *p, uint32_t lba, uint32_t size, int is_dir, const char *name, size_t nl)
{
    size_t len = 33 + nl + ((33 + nl) & 1);
    memset(p, 0, len);
    p[0] = (uint8_t)len;
    put_le32_be32(p + 2, lba);
    put_le32_be32(p + 10, size);
    p[25] = is_dir ? 2 : 0;
    p[32] = (uint8_t)nl;
    memcpy(p + 33, name, nl);
    return len;
}

/* sectors: 16 PVD, 17 terminator, 18 root, 19 GAME dir, 20-21 exe, 22-24 dll */
static void build_iso(uint8_t img[25][2048])
{
    memset(img, 0, 25 * 2048);
    uint8_t *pvd = img[16];
    pvd[0] = 1;
    memcpy(pvd + 1, "CD001", 5);
    pvd[6] = 1;
    dirrec(pvd + 156, 18, 2048, 1, "\0", 1);
    img[17][0] = 255;
    memcpy(img[17] + 1, "CD001", 5);
    size_t o = 0;
    o += dirrec(img[18] + o, 18, 2048, 1, "\0", 1);
    o += dirrec(img[18] + o, 18, 2048, 1, "\1", 1);
    o += dirrec(img[18] + o, 19, 2048, 1, "GAME", 4);
    o = 0;
    o += dirrec(img[19] + o, 19, 2048, 1, "\0", 1);
    o += dirrec(img[19] + o, 18, 2048, 1, "\1", 1);
    o += dirrec(img[19] + o, 20, sizeof exe_data, 0, "BLIZZARD.EXE;1", 14);
    o += dirrec(img[19] + o, 22, sizeof dll_data, 0, "SB_CDRES.DLL;1", 14);
    memcpy(img[20], exe_data, sizeof exe_data);
    memcpy(img[22], dll_data, sizeof dll_data);
}

static void write_file(const char *path, const void *data, size_t len)
{
    FILE *fp = fopen(path, "wb");
    if (!fp || fwrite(data, 1, len, fp) != len) {
        perror(path);
        exit(1);
    }
    fclose(fp);
}

static void expect_ok(const char *path)
{
    GameFiles g;
    char err[512] = "";
    int rc = gamefiles_read(path, &g, err, sizeof err);
    if (rc) fprintf(stderr, "%s: %s\n", path, err);
    CHECK(rc == 0);
    if (rc) return;
    CHECK(g.exe_len == sizeof exe_data && !memcmp(g.exe, exe_data, sizeof exe_data));
    CHECK(g.dll_len == sizeof dll_data && !memcmp(g.dll, dll_data, sizeof dll_data));
    /* synthetic contents: the version check must refuse them */
    CHECK(gamefiles_check(&g, err, sizeof err) == -1 && strstr(err, "different version"));
    gamefiles_free(&g);
}

static void expect_err(const char *path, const char *words)
{
    GameFiles g;
    char err[512] = "";
    CHECK(gamefiles_read(path, &g, err, sizeof err) == -1);
    if (!strstr(err, words)) fprintf(stderr, "%s: unexpected message: %s\n", path, err);
    CHECK(strstr(err, words) != NULL);
    CHECK(g.exe == NULL && g.dll == NULL);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: gamefiles_test SCRATCH_DIR\n");
        return 2;
    }
    const char *dir = argv[1];
    char p[1024], q[2048];
    mkdir(dir, 0755);

    /* SHA-256: FIPS 180-4 test vectors */
    char hex[65];
    sha256_hex("abc", 3, hex);
    CHECK(!strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    sha256_hex("", 0, hex);
    CHECK(!strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    const char *two = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha256_hex(two, strlen(two), hex);
    CHECK(!strcmp(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));

    for (size_t i = 0; i < sizeof exe_data; i++) exe_data[i] = (uint8_t)(i * 7 + 1);
    for (size_t i = 0; i < sizeof dll_data; i++) dll_data[i] = (uint8_t)(i * 13 + 5);

    /* a folder, files two levels down in lower case */
    snprintf(p, sizeof p, "%s/folder", dir);
    mkdir(p, 0755);
    snprintf(p, sizeof p, "%s/folder/Disc", dir);
    mkdir(p, 0755);
    snprintf(p, sizeof p, "%s/folder/Disc/blizzard", dir);
    mkdir(p, 0755);
    snprintf(q, sizeof q, "%s/blizzard.exe", p);
    write_file(q, exe_data, sizeof exe_data);
    snprintf(q, sizeof q, "%s/Sb_CdRes.dll", p);
    write_file(q, dll_data, sizeof dll_data);
    snprintf(p, sizeof p, "%s/folder", dir);
    expect_ok(p);
    expect_ok(q);  /* one of the files: its folder is used */

    /* a plain ISO */
    static uint8_t img[25][2048];
    build_iso(img);
    snprintf(p, sizeof p, "%s/game.iso", dir);
    write_file(p, img, sizeof img);
    expect_ok(p);

    /* a raw mode-1 BIN (2352-byte sectors) and its CUE */
    static uint8_t raw[25][2352];
    for (int s = 0; s < 25; s++) {
        static const uint8_t sync[12] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
        memset(raw[s], 0, 2352);
        memcpy(raw[s], sync, 12);
        raw[s][15] = 1;
        memcpy(raw[s] + 16, img[s], 2048);
    }
    snprintf(p, sizeof p, "%s/Spelling Blizzard (Track 1).bin", dir);
    write_file(p, raw, sizeof raw);
    expect_ok(p);
    snprintf(p, sizeof p, "%s/game.cue", dir);
    const char *cue = "FILE \"Spelling Blizzard (Track 1).bin\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n";
    write_file(p, cue, strlen(cue));
    expect_ok(p);

    /* errors */
    snprintf(p, sizeof p, "%s/nothing-here", dir);
    expect_err(p, "doesn't exist");
    snprintf(q, sizeof q, "%s/folder/Disc/blizzard/Sb_CdRes.dll", dir);
    remove(q);
    snprintf(p, sizeof p, "%s/folder", dir);
    expect_err(p, "SB_CDRES.DLL isn't in");
    snprintf(p, sizeof p, "%s/junk.iso", dir);
    write_file(p, "not a CD image", 14);
    expect_err(p, "isn't a CD image");
    snprintf(p, sizeof p, "%s/short.iso", dir);
    write_file(p, img, 21 * 2048);  /* cut off inside the EXE */
    expect_err(p, "damaged or cut short");
    snprintf(p, sizeof p, "%s/game.chd", dir);
    write_file(p, "MComprHD", 8);
    expect_err(p, "CHD images aren't supported");

    if (failures) fprintf(stderr, "%d check(s) failed\n", failures);
    else printf("gamefiles: all checks passed\n");
    return failures ? 1 : 0;
}

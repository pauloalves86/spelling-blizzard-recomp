/* Win16 KERNEL and DOS (INT 21h) services for the recompiled game. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "api.h"
#include "hostfs.h"
#include "loader.h"
#include "win16.h"

int g_api_trace = 0;

void api_log(const char *fmt, ...)
{
    va_list ap;
    if (!g_api_trace) {
        return;
    }
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

_Noreturn void api_unimplemented(const char *module, const char *name, int ordinal)
{
    uint16_t ret_ip = RW(SS, SP), ret_cs = RW(SS, (uint16_t)(SP + 2));
    recomp_fatal("unimplemented API %s.%s (ordinal %d) called from %04x:%04x",
                 module, name, ordinal, ret_cs, ret_ip);
}

/* ------------------------------------------------------------ strings */

size_t far_strlen(uint32_t fp)
{
    size_t n = 0;
    if (!fp) {
        return 0;
    }
    while (RB((uint16_t)(fp >> 16), (uint16_t)((uint16_t)fp + n)) != 0) {
        ++n;
    }
    return n;
}

void far_to_c(uint32_t fp, char *out, size_t cap)
{
    size_t i = 0;
    if (fp) {
        for (; i + 1 < cap; ++i) {
            char c = (char)RB((uint16_t)(fp >> 16), (uint16_t)((uint16_t)fp + i));
            if (!c) {
                break;
            }
            out[i] = c;
        }
    }
    out[i] = 0;
}

void c_to_far(const char *s, uint32_t fp, size_t cap)
{
    size_t i;
    if (!fp || !cap) {
        return;
    }
    for (i = 0; i + 1 < cap && s[i]; ++i) {
        WB((uint16_t)(fp >> 16), (uint16_t)((uint16_t)fp + i), (uint8_t)s[i]);
    }
    WB((uint16_t)(fp >> 16), (uint16_t)((uint16_t)fp + i), 0);
}

/* ------------------------------------------------------------ process */

void api_KERNEL_INITTASK(void)
{
    NEModule *m = &g_exe;
    uint16_t stack_size = BX;
    uint16_t stacktop = (uint16_t)(m->dgroup_size - stack_size + 150);
    (void)CX;
    /* INSTANCEDATA at DS:0 */
    WW(DS, 0x0A, stacktop);              /* pStackTop (lowest usable) */
    WW(DS, 0x0C, (uint16_t)(SP - 16));   /* pStackMin */
    WW(DS, 0x0E, (uint16_t)(SP - 16));   /* pStackBot */
    AX = 1;
    BX = 0x81;
    CX = m->stack;
    DX = m->ncmdshow;
    SI = 0;
    DI = DGROUP_SEL;
    ES = g_psp_sel;
    api_ret(0);
}

void api_KERNEL_WAITEVENT(void) { AX = 0; api_ret(2); }
void api_USER_INITAPP(void) { AX = 1; api_ret(2); }
void api_KERNEL_LOCKSEGMENT(void)
{
    ArgReader a = args_pascal(2);
    uint16_t s = arg_w(&a);
    AX = s == 0xFFFF ? DS : s;
    api_ret(2);
}
void api_KERNEL_GETVERSION(void) { AX = 0x0A03; DX = 0x0600; api_ret(0); } /* Windows 3.10 on DOS 6.0 */
void api_KERNEL_GETDOSENVIRONMENT(void) { ret_d(mkfar(g_env_sel, 0)); api_ret(0); }

_Noreturn static void do_exit(int code)
{
    win16_shutdown();
    fflush(stdout);
    exit(code);
}

void api_KERNEL_FATALEXIT(void)
{
    ArgReader a = args_pascal(2);
    recomp_fatal("FatalExit(%d)", arg_i(&a));
}

void api_KERNEL_FATALAPPEXIT(void)
{
    ArgReader a = args_pascal(6);
    char msg[256];
    arg_w(&a);
    far_to_c(arg_d(&a), msg, sizeof(msg));
    fprintf(stderr, "FatalAppExit: %s\n", msg);
    do_exit(3);
}

void api_KERNEL_GETMODULEFILENAME(void)
{
    ArgReader a = args_pascal(8);
    uint16_t h = arg_w(&a);
    uint32_t buf = arg_d(&a);
    int16_t n = arg_i(&a);
    const char *path = (h == g_dll.hmodule && h) ? "D:\\SB_CDRES.DLL" : "C:\\SB\\BLIZZARD.EXE";
    c_to_far(path, buf, (size_t)n);
    AX = (uint16_t)strlen(path);
    api_ret(8);
}

void api_KERNEL_GETWINDOWSDIRECTORY(void)
{
    ArgReader a = args_pascal(6);
    uint32_t buf = arg_d(&a);
    uint16_t n = arg_w(&a);
    c_to_far("C:\\WINDOWS", buf, n);
    AX = 10;
    api_ret(6);
}

void api_KERNEL_GETSYSTEMDIRECTORY(void)
{
    ArgReader a = args_pascal(6);
    uint32_t buf = arg_d(&a);
    uint16_t n = arg_w(&a);
    c_to_far("C:\\WINDOWS\\SYSTEM", buf, n);
    AX = 17;
    api_ret(6);
}

/* ------------------------------------------------------------ memory */

void api_KERNEL_GLOBALALLOC(void)
{
    ArgReader a = args_pascal(6);
    uint16_t flags = arg_w(&a);
    uint32_t size = arg_d(&a);
    AX = global_alloc(flags, size);
    api_log("GlobalAlloc(%04x, %u) = %04x", flags, size, AX);
    api_ret(6);
}

void api_KERNEL_GLOBALREALLOC(void)
{
    ArgReader a = args_pascal(8);
    uint16_t h = arg_w(&a);
    uint32_t size = arg_d(&a);
    uint16_t flags = arg_w(&a);
    if (flags & 0x80) { /* GMEM_MODIFY */
        AX = h;
    } else {
        AX = global_realloc(h, size);
    }
    api_ret(8);
}

void api_KERNEL_GLOBALFREE(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    AX = global_free(h) ? 0 : h;
    api_ret(2);
}

void api_KERNEL_GLOBALLOCK(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    if (h && global_size(h)) {
        ret_d(mkfar(global_handle(h), 0));
    } else if (h && SEGBASE[h >> 3]) {
        ret_d(mkfar(h, 0)); /* a fixed segment (e.g. DGROUP) */
    } else {
        ret_d(0);
    }
    api_ret(2);
}

void api_KERNEL_GLOBALUNLOCK(void) { AX = 0; api_ret(2); }

void api_KERNEL_GLOBALSIZE(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    ret_d(global_size(h));
    api_ret(2);
}

void api_KERNEL_GLOBALHANDLE(void)
{
    ArgReader a = args_pascal(2);
    uint16_t s = arg_w(&a);
    uint16_t h = global_handle(s);
    ret_d(h ? mkfar(h, h) : 0);
    api_ret(2);
}

void api_KERNEL_GLOBALCOMPACT(void) { ret_d(32u * 1024u * 1024u); api_ret(4); }

void api_KERNEL_HMEMCPY(void)
{
    ArgReader a = args_pascal(12);
    uint32_t dst = arg_d(&a);
    uint32_t src = arg_d(&a);
    uint32_t n = arg_d(&a);
    /* huge pointers: sel advances by __AHINCR (8) per 64K, contiguous in MEM */
    memmove(farp(dst), farp(src), n);
    api_ret(12);
}

void api_KERNEL_LSTRCPY(void)
{
    ArgReader a = args_pascal(8);
    uint32_t dst = arg_d(&a);
    uint32_t src = arg_d(&a);
    size_t n = far_strlen(src);
    memmove(farp(dst), farp(src), n + 1);
    ret_d(dst);
    api_ret(8);
}

/* ------------------------------------------------------------ files */

#define MAX_FILES 64
static FILE *g_files[MAX_FILES];

static int file_alloc(FILE *fp)
{
    int i;
    for (i = 5; i < MAX_FILES; ++i) {
        if (!g_files[i]) {
            g_files[i] = fp;
            return i;
        }
    }
    fclose(fp);
    return -1;
}

static FILE *file_get(uint16_t h) { return h < MAX_FILES ? g_files[h] : NULL; }

static int file_open(const char *dos, int mode, int create)
{
    char host[1024];
    FILE *fp = NULL;
    if (create) {
        hostfs_resolve_create(dos, host, sizeof(host));
        fp = fopen(host, "w+b");
    } else if (hostfs_resolve_existing(dos, host, sizeof(host)) == 0) {
        int acc = mode & 3;
        if (acc == 0) {
            fp = fopen(host, "rb");
        } else {
            /* writes to read-only game files go to a copy in the data dir */
            char w[1024];
            hostfs_resolve_create(dos, w, sizeof(w));
            if (strcmp(w, host) != 0) {
                FILE *in = fopen(host, "rb"), *out = fopen(w, "wb");
                char buf[8192];
                size_t n;
                while (in && out && (n = fread(buf, 1, sizeof(buf), in)) > 0) {
                    fwrite(buf, 1, n, out);
                }
                if (in) fclose(in);
                if (out) fclose(out);
            }
            fp = fopen(w, "r+b");
        }
    }
    api_log("open(%s, mode %x%s) -> %s", dos, mode, create ? ", create" : "", fp ? host : "FAIL");
    return fp ? file_alloc(fp) : -1;
}

void api_KERNEL_OPENFILE(void)
{
    ArgReader a = args_pascal(10);
    uint32_t name = arg_d(&a);
    uint32_t ofs = arg_d(&a);
    uint16_t style = arg_w(&a);
    char dos[260];
    int h = -1;
    far_to_c(name, dos, sizeof(dos));
    if (style & 0x4000) { /* OF_EXIST */
        char host[1024];
        h = hostfs_resolve_existing(dos, host, sizeof(host)) == 0 ? 1 : -1;
    } else if (style & 0x0200) { /* OF_DELETE */
        char host[1024];
        h = (hostfs_resolve_existing(dos, host, sizeof(host)) == 0 && remove(host) == 0) ? 1 : -1;
    } else {
        h = file_open(dos, style & 3, (style & 0x1000) != 0 /* OF_CREATE */);
    }
    if (ofs) {
        uint8_t *o = (uint8_t *)farp(ofs);
        memset(o, 0, 8);
        o[0] = 136;
        o[2] = (uint8_t)(h < 0 ? 2 : 0);
        snprintf((char *)o + 8, 128, "%s", dos);
    }
    AX = (uint16_t)h;
    api_ret(10);
}

void api_KERNEL__HREAD(void)
{
    ArgReader a = args_pascal(10);
    uint16_t h = arg_w(&a);
    uint32_t buf = arg_d(&a);
    uint32_t n = arg_d(&a);
    FILE *fp = file_get(h);
    size_t got = fp ? fread(farp(buf), 1, n, fp) : 0;
    ret_d(fp ? (uint32_t)got : 0xFFFFFFFFu);
    api_ret(10);
}

void api_KERNEL__HWRITE(void)
{
    ArgReader a = args_pascal(10);
    uint16_t h = arg_w(&a);
    uint32_t buf = arg_d(&a);
    uint32_t n = arg_d(&a);
    FILE *fp = file_get(h);
    size_t put = fp ? fwrite(farp(buf), 1, n, fp) : 0;
    if (fp) fflush(fp);
    ret_d(fp ? (uint32_t)put : 0xFFFFFFFFu);
    api_ret(10);
}

/* ------------------------------------------------------------ DOS */

static void dos_ok(void) { cpu.cf = 0; }
static void dos_err(uint16_t code) { cpu.cf = 1; AX = code; }

/* ---- directory search (findfirst/findnext into the DTA) ---- */
static uint32_t g_dta;
typedef struct FindState {
    char names[256][16];
    uint8_t attrs[256];
    uint32_t sizes[256];
    int n, i;
} FindState;
static FindState g_find;

static int dos_match(const char *pat, const char *name)
{
    /* DOS 8.3 wildcard match on name and extension separately */
    char pn[16] = "", pe[8] = "", nn[16] = "", ne[8] = "";
    const char *d;
    int i;
    d = strchr(pat, '.');
    if (d) { snprintf(pn, sizeof(pn), "%.*s", (int)(d - pat), pat); snprintf(pe, sizeof(pe), "%s", d + 1); }
    else { snprintf(pn, sizeof(pn), "%s", pat); }
    d = strchr(name, '.');
    if (d) { snprintf(nn, sizeof(nn), "%.*s", (int)(d - name), name); snprintf(ne, sizeof(ne), "%s", d + 1); }
    else { snprintf(nn, sizeof(nn), "%s", name); }
    for (i = 0; i < 2; ++i) {
        const char *p = i ? pe : pn, *n = i ? ne : nn;
        if (i == 1 && !d && !strchr(pat, '.')) break;
        while (*p) {
            if (*p == '*') { n = ""; break; }
            if (*p == '?') { if (*n) ++n; ++p; continue; }
            if (toupper((unsigned char)*p) != toupper((unsigned char)*n)) return 0;
            ++p; ++n;
        }
        if (*n) return 0;
    }
    return 1;
}

static void find_fill(void)
{
    uint8_t *d = (uint8_t *)farp(g_dta);
    if (g_find.i >= g_find.n) { dos_err(18); return; }
    memset(d, 0, 43);
    d[21] = g_find.attrs[g_find.i];
    memcpy(d + 26, &g_find.sizes[g_find.i], 4);
    { uint16_t t = 0, dt = (uint16_t)(((1994 - 1980) << 9) | (1 << 5) | 1); memcpy(d + 22, &t, 2); memcpy(d + 24, &dt, 2); }
    snprintf((char *)d + 30, 13, "%s", g_find.names[g_find.i]);
    g_find.i++;
    dos_ok();
}

static void find_first(const char *pattern, uint16_t attr)
{
    char dir[260], host[1024];
    const char *mask = pattern;
    const char *p;
    DIR *dd;
    struct dirent *e;
    for (p = pattern; *p; ++p) if (*p == '\\' || *p == ':') mask = p + 1;
    snprintf(dir, sizeof(dir), "%.*s", (int)(mask - pattern), pattern);
    memset(&g_find, 0, sizeof(g_find));
    if (hostfs_resolve_dir(dir[0] ? dir : ".", host, sizeof(host)) != 0 || !(dd = opendir(host))) {
        dos_err(3);
        return;
    }
    while ((e = readdir(dd)) != NULL && g_find.n < 256) {
        char full[1300];
        struct stat st;
        if (e->d_name[0] == '.' || strlen(e->d_name) > 12) continue;
        if (!dos_match(mask, e->d_name)) continue;
        snprintf(full, sizeof(full), "%s/%s", host, e->d_name);
        if (stat(full, &st) != 0) continue;
        if (S_ISDIR(st.st_mode) && !(attr & 0x10)) continue;
        snprintf(g_find.names[g_find.n], 16, "%s", e->d_name);
        { char *c; for (c = g_find.names[g_find.n]; *c; ++c) *c = (char)toupper((unsigned char)*c); }
        g_find.attrs[g_find.n] = S_ISDIR(st.st_mode) ? 0x10 : 0x20;
        g_find.sizes[g_find.n] = (uint32_t)st.st_size;
        g_find.n++;
    }
    closedir(dd);
    api_log("findfirst(%s, %x) -> %d entries", pattern, attr, g_find.n);
    if (!g_find.n) { dos_err(18); return; }
    find_fill();
}

static void find_next(void) { find_fill(); }

void dos_int21(void)
{
    uint8_t fn = AH;
    char path[260];
    FILE *fp;
    switch (fn) {
        case 0x30: AX = 0x0006; BX = 0; CX = 0; dos_ok(); return; /* DOS 6.0 */
        case 0x19: AL = 2; dos_ok(); return;                        /* drive C: */
        case 0x0E: AL = 26; dos_ok(); return;
        case 0x25: dos_ok(); return;                                /* set vector: ignore */
        case 0x35: ES = 0; BX = 0; dos_ok(); return;                /* get vector */
        case 0x2A: {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            CX = (uint16_t)(tm->tm_year + 1900); DH = (uint8_t)(tm->tm_mon + 1); DL = (uint8_t)tm->tm_mday; AL = (uint8_t)tm->tm_wday;
            dos_ok(); return;
        }
        case 0x2C: {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            CH = (uint8_t)tm->tm_hour; CL = (uint8_t)tm->tm_min; DH = (uint8_t)tm->tm_sec; DL = 0;
            dos_ok(); return;
        }
        case 0x39: { /* mkdir (inside the data dir) */
            char host[1024];
            far_to_c(mkfar(DS, DX), path, sizeof(path));
            hostfs_resolve_create(path, host, sizeof(host));
            if (mkdir(host, 0755) == 0) dos_ok();
            else dos_err(5);
            return;
        }
        case 0x3A: /* rmdir */
        case 0x3B: /* chdir: paths are resolved from the install dir anyway */
            dos_ok();
            return;
        case 0x36: AX = 8; BX = 0x7FFF; CX = 512; DX = 0xFFFF; return; /* free space: ~128 MB */
        case 0x1A: g_dta = mkfar(DS, DX); dos_ok(); return;
        case 0x2F: if (!g_dta) g_dta = mkfar(g_psp_sel, 0x80); ES = (uint16_t)(g_dta >> 16); BX = (uint16_t)g_dta; dos_ok(); return;
        case 0x4E: /* findfirst */
            far_to_c(mkfar(DS, DX), path, sizeof(path));
            if (!g_dta) g_dta = mkfar(g_psp_sel, 0x80);
            find_first(path, CX);
            return;
        case 0x4F: /* findnext */
            if (!g_dta) g_dta = mkfar(g_psp_sel, 0x80);
            find_next();
            return;
        case 0x3C: /* create */
            far_to_c(mkfar(DS, DX), path, sizeof(path));
            { int h = file_open(path, 2, 1); if (h < 0) dos_err(3); else { AX = (uint16_t)h; dos_ok(); } }
            return;
        case 0x3D: /* open */
            far_to_c(mkfar(DS, DX), path, sizeof(path));
            { int h = file_open(path, AL & 3, 0); if (h < 0) dos_err(2); else { AX = (uint16_t)h; dos_ok(); } }
            return;
        case 0x3E:
            fp = file_get(BX);
            if (fp) { fclose(fp); g_files[BX] = NULL; dos_ok(); }
            else if (BX < 5) dos_ok();
            else dos_err(6);
            return;
        case 0x3F: /* read */
            fp = file_get(BX);
            if (!fp) { if (BX < 5) { AX = 0; dos_ok(); } else dos_err(6); return; }
            AX = (uint16_t)fread(FARPTR(DS, DX), 1, CX, fp);
            dos_ok();
            return;
        case 0x40: /* write */
            if (BX == 1 || BX == 2) {
                fwrite(FARPTR(DS, DX), 1, CX, stderr);
                AX = CX; dos_ok(); return;
            }
            fp = file_get(BX);
            if (!fp) { dos_err(6); return; }
            AX = (uint16_t)fwrite(FARPTR(DS, DX), 1, CX, fp);
            fflush(fp);
            dos_ok();
            return;
        case 0x41: {
            char host[1024];
            far_to_c(mkfar(DS, DX), path, sizeof(path));
            if (hostfs_resolve_existing(path, host, sizeof(host)) == 0 && remove(host) == 0) dos_ok();
            else dos_err(2);
            return;
        }
        case 0x42: { /* lseek */
            long off = (long)(int32_t)(((uint32_t)CX << 16) | DX);
            long pos;
            fp = file_get(BX);
            if (!fp) { dos_err(6); return; }
            fseek(fp, off, AL == 0 ? SEEK_SET : AL == 1 ? SEEK_CUR : SEEK_END);
            pos = ftell(fp);
            AX = (uint16_t)pos; DX = (uint16_t)((uint32_t)pos >> 16);
            dos_ok();
            return;
        }
        case 0x43: {
            char host[1024];
            far_to_c(mkfar(DS, DX), path, sizeof(path));
            if (hostfs_resolve_existing(path, host, sizeof(host)) == 0) { CX = 0x20; dos_ok(); }
            else dos_err(2);
            return;
        }
        case 0x44: /* ioctl */
            if (AL == 0) {
                DX = (BX < 5) ? 0x80D3 : 0x0002;
                dos_ok();
                return;
            }
            dos_err(1);
            return;
        case 0x47: /* get cwd */
            c_to_far("SB", mkfar(DS, SI), 64);
            dos_ok();
            return;
        case 0x4C:
            fprintf(stderr, "[recomp] program exited with code %u\n", AL);
            do_exit(AL);
        default:
            recomp_fatal("unimplemented INT 21h AH=%02xh", fn);
    }
}

void api_KERNEL_DOS3CALL(void)
{
    /* DOS3CALL is a far call with register arguments; return address on stack */
    dos_int21();
    api_ret(0);
}

/* ------------------------------------------------------------ profiles (INI) */

static void ini_path(const char *file, char *out, size_t cap)
{
    hostfs_resolve_create(file, out, cap);
}

/* Reads [app] key from an INI file; returns 1 if found. */
static int ini_get(const char *file, const char *app, const char *key, char *out, size_t cap)
{
    char host[1024], line[1024];
    FILE *fp;
    int in_sec = 0;
    ini_path(file, host, sizeof(host));
    fp = fopen(host, "r");
    if (!fp) {
        char ro[1024];
        if (hostfs_resolve_existing(file, ro, sizeof(ro)) != 0 || !(fp = fopen(ro, "r"))) {
            return 0;
        }
    }
    while (fgets(line, sizeof(line), fp)) {
        char *p = line, *e;
        line[strcspn(line, "\r\n")] = 0;
        while (isspace((unsigned char)*p)) ++p;
        if (*p == '[') {
            e = strchr(p, ']');
            if (e) *e = 0;
            in_sec = strcasecmp(p + 1, app) == 0;
            continue;
        }
        if (in_sec && (e = strchr(p, '=')) != NULL) {
            *e = 0;
            {
                char *k = p, *ke = e;
                while (ke > k && isspace((unsigned char)ke[-1])) *--ke = 0;
                if (strcasecmp(k, key) == 0) {
                    snprintf(out, cap, "%s", e + 1);
                    fclose(fp);
                    return 1;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

static void ini_set(const char *file, const char *app, const char *key, const char *val)
{
    char host[1024], line[1024];
    char *outbuf = NULL;
    size_t outlen = 0;
    FILE *fp, *mem;
    int in_sec = 0, done = 0, found_sec = 0;
    ini_path(file, host, sizeof(host));
    mem = open_memstream(&outbuf, &outlen);
    fp = fopen(host, "r");
    while (fp && fgets(line, sizeof(line), fp)) {
        char tmp[1024];
        char *p = tmp, *e;
        snprintf(tmp, sizeof(tmp), "%s", line);
        tmp[strcspn(tmp, "\r\n")] = 0;
        while (isspace((unsigned char)*p)) ++p;
        if (*p == '[') {
            if (in_sec && !done && key && val) {
                fprintf(mem, "%s=%s\n", key, val);
                done = 1;
            }
            e = strchr(p, ']');
            if (e) *e = 0;
            in_sec = strcasecmp(p + 1, app) == 0;
            if (in_sec) found_sec = 1;
            if (in_sec && !key) continue; /* delete section */
        } else if (in_sec && (e = strchr(p, '=')) != NULL) {
            *e = 0;
            if (!key) continue;
            if (strcasecmp(p, key) == 0) {
                if (val && !done) fprintf(mem, "%s=%s\n", key, val);
                done = 1;
                continue;
            }
        } else if (in_sec && !key) {
            continue;
        }
        fputs(line, mem);
        if (!strchr(line, '\n')) fputc('\n', mem);
    }
    if (fp) fclose(fp);
    if (!done && key && val) {
        if (!found_sec || !in_sec) fprintf(mem, "[%s]\n", app);
        fprintf(mem, "%s=%s\n", key, val);
    }
    fclose(mem);
    fp = fopen(host, "w");
    if (fp) {
        fwrite(outbuf, 1, outlen, fp);
        fclose(fp);
    }
    free(outbuf);
}

static void profile_get_string(uint32_t app_p, uint32_t key_p, uint32_t def_p, uint32_t ret_p, uint16_t size,
                               const char *file)
{
    char app[128], key[128], def[512], val[512];
    far_to_c(app_p, app, sizeof(app));
    far_to_c(def_p, def, sizeof(def));
    if (!key_p) {
        /* enumerate keys: not needed by the game; return empty list */
        if (size >= 2) { WB((uint16_t)(ret_p >> 16), (uint16_t)ret_p, 0); WB((uint16_t)(ret_p >> 16), (uint16_t)((uint16_t)ret_p + 1), 0); }
        AX = 0;
        return;
    }
    far_to_c(key_p, key, sizeof(key));
    if (!ini_get(file, app, key, val, sizeof(val))) {
        snprintf(val, sizeof(val), "%s", def);
    }
    c_to_far(val, ret_p, size);
    AX = (uint16_t)(strlen(val) < size ? strlen(val) : (size ? size - 1u : 0u));
    api_log("GetProfileString(%s, [%s] %s) = \"%s\"", file, app, key, val);
}

void api_KERNEL_GETPROFILESTRING(void)
{
    ArgReader a = args_pascal(18);
    uint32_t app = arg_d(&a), key = arg_d(&a), def = arg_d(&a), ret = arg_d(&a);
    uint16_t size = arg_w(&a);
    profile_get_string(app, key, def, ret, size, "WIN.INI");
    api_ret(18);
}

void api_KERNEL_GETPRIVATEPROFILESTRING(void)
{
    ArgReader a = args_pascal(22);
    uint32_t app = arg_d(&a), key = arg_d(&a), def = arg_d(&a), ret = arg_d(&a);
    uint16_t size = arg_w(&a);
    char file[260];
    far_to_c(arg_d(&a), file, sizeof(file));
    profile_get_string(app, key, def, ret, size, file);
    api_ret(22);
}

void api_KERNEL_GETPRIVATEPROFILEINT(void)
{
    ArgReader a = args_pascal(14);
    char app[128], key[128], file[260], val[64];
    uint32_t app_p = arg_d(&a), key_p = arg_d(&a);
    int16_t def = arg_i(&a);
    far_to_c(arg_d(&a), file, sizeof(file));
    far_to_c(app_p, app, sizeof(app));
    far_to_c(key_p, key, sizeof(key));
    AX = ini_get(file, app, key, val, sizeof(val)) ? (uint16_t)atoi(val) : (uint16_t)def;
    api_log("GetPrivateProfileInt(%s, [%s] %s) = %d", file, app, key, (int16_t)AX);
    api_ret(14);
}

static void profile_write(uint32_t app_p, uint32_t key_p, uint32_t val_p, const char *file)
{
    char app[128], key[128], val[512];
    far_to_c(app_p, app, sizeof(app));
    far_to_c(key_p, key, sizeof(key));
    far_to_c(val_p, val, sizeof(val));
    ini_set(file, app, key_p ? key : NULL, val_p ? val : NULL);
    AX = 1;
}

void api_KERNEL_WRITEPROFILESTRING(void)
{
    ArgReader a = args_pascal(12);
    uint32_t app = arg_d(&a), key = arg_d(&a), val = arg_d(&a);
    profile_write(app, key, val, "WIN.INI");
    api_ret(12);
}

void api_KERNEL_WRITEPRIVATEPROFILESTRING(void)
{
    ArgReader a = args_pascal(16);
    uint32_t app = arg_d(&a), key = arg_d(&a), val = arg_d(&a);
    char file[260];
    far_to_c(arg_d(&a), file, sizeof(file));
    profile_write(app, key, val, file);
    api_ret(16);
}

/* ------------------------------------------------------------ modules & resources */

#define DLL_HMODULE 0x0E10u

static NEModule *module_for(uint16_t h)
{
    if (h == DLL_HMODULE && g_dll.raw) {
        return &g_dll;
    }
    return &g_exe;
}

void api_KERNEL_LOADLIBRARY(void)
{
    ArgReader a = args_pascal(4);
    char name[260], host[1024];
    far_to_c(arg_d(&a), name, sizeof(name));
    if (g_dll.raw) {
        AX = DLL_HMODULE;
    } else if (hostfs_resolve_existing(name, host, sizeof(host)) == 0 && loader_load_dll(host, DLL_HMODULE) == 0) {
        AX = DLL_HMODULE;
    } else {
        AX = 2; /* file not found */
    }
    api_log("LoadLibrary(%s) = %04x", name, AX);
    api_ret(4);
}

void api_KERNEL_FREELIBRARY(void) { api_ret(2); }

/* HRSRC encodes (module, entry offset in resource table). */
typedef struct RsrcRef {
    NEModule *m;
    uint32_t entry; /* file offset of the 12-byte NAMEINFO */
} RsrcRef;
#define MAX_RSRC 4096
static RsrcRef g_rsrc[MAX_RSRC];
static int g_nrsrc = 1;

static int rsrc_name_match(NEModule *m, uint16_t id, uint32_t name_fp)
{
    if ((name_fp >> 16) == 0) {
        return id == (0x8000u | (uint16_t)name_fp);
    } else {
        char want[128];
        far_to_c(name_fp, want, sizeof(want));
        if (want[0] == '#') {
            return id == (0x8000u | (uint16_t)atoi(want + 1));
        }
        if (id & 0x8000u) {
            return 0;
        }
        {
            const uint8_t *s = m->raw + m->rsrctab + id;
            return strlen(want) == s[0] && strncasecmp(want, (const char *)s + 1, s[0]) == 0;
        }
    }
}

void api_KERNEL_FINDRESOURCE(void)
{
    ArgReader a = args_pascal(10);
    uint16_t h = arg_w(&a);
    uint32_t name = arg_d(&a);
    uint32_t type = arg_d(&a);
    NEModule *m = module_for(h);
    const uint8_t *p = m->raw + m->rsrctab + 2;
    AX = 0;
    while (p[0] | p[1]) {
        uint16_t tid = (uint16_t)(p[0] | (p[1] << 8));
        uint16_t cnt = (uint16_t)(p[2] | (p[3] << 8));
        const uint8_t *e = p + 8;
        if (rsrc_name_match(m, tid, type)) {
            unsigned k;
            for (k = 0; k < cnt; ++k, e += 12) {
                uint16_t id = (uint16_t)(e[6] | (e[7] << 8));
                if (rsrc_name_match(m, id, name)) {
                    int i;
                    for (i = 1; i < g_nrsrc; ++i) {
                        if (g_rsrc[i].m == m && g_rsrc[i].entry == (uint32_t)(e - m->raw)) break;
                    }
                    if (i == g_nrsrc && g_nrsrc < MAX_RSRC) {
                        g_rsrc[g_nrsrc].m = m;
                        g_rsrc[g_nrsrc].entry = (uint32_t)(e - m->raw);
                        g_nrsrc++;
                    }
                    AX = (uint16_t)i;
                    break;
                }
            }
            break;
        }
        p = e + 12u * cnt;
    }
    if (g_api_trace) {
        api_log("FindResource(%04x, %08x, %08x) = %u", h, name, type, AX);
    }
    api_ret(10);
}

static void rsrc_loc(uint16_t hr, uint32_t *off, uint32_t *len)
{
    RsrcRef *r = &g_rsrc[hr];
    const uint8_t *e = r->m->raw + r->entry;
    unsigned shift = (unsigned)(r->m->raw[r->m->rsrctab] | (r->m->raw[r->m->rsrctab + 1] << 8));
    *off = (uint32_t)(e[0] | (e[1] << 8)) << shift;
    *len = (uint32_t)(e[2] | (e[3] << 8)) << shift;
}

/* Loaded resources: HGLOBAL -> HRSRC, so repeated loads share one block. */
static uint16_t g_rsrc_loaded[MAX_RSRC];

void api_KERNEL_LOADRESOURCE(void)
{
    ArgReader a = args_pascal(4);
    uint16_t h = arg_w(&a);
    uint16_t hr = arg_w(&a);
    uint32_t off, len;
    (void)h;
    if (hr == 0 || hr >= g_nrsrc) {
        AX = 0;
        api_ret(4);
        return;
    }
    if (g_rsrc_loaded[hr] && global_size(g_rsrc_loaded[hr])) {
        AX = g_rsrc_loaded[hr];
        api_ret(4);
        return;
    }
    rsrc_loc(hr, &off, &len);
    AX = global_alloc(0, len);
    memcpy(FARPTR(AX, 0), g_rsrc[hr].m->raw + off, len);
    g_rsrc_loaded[hr] = AX;
    api_ret(4);
}

void api_KERNEL_LOCKRESOURCE(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    ret_d(h ? mkfar(h, 0) : 0);
    api_ret(2);
}

void api_KERNEL_FREERESOURCE(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    int i;
    for (i = 1; i < g_nrsrc; ++i) {
        if (g_rsrc_loaded[i] == h) {
            g_rsrc_loaded[i] = 0;
        }
    }
    global_free(h);
    AX = 0;
    api_ret(2);
}

void api_KERNEL_SIZEOFRESOURCE(void)
{
    ArgReader a = args_pascal(4);
    uint16_t h = arg_w(&a);
    uint16_t hr = arg_w(&a);
    uint32_t off = 0, len = 0;
    (void)h;
    if (hr && hr < g_nrsrc) {
        rsrc_loc(hr, &off, &len);
    }
    ret_d(len);
    api_ret(4);
}

/* Host access for the USER layer (cursors). */
int kernel_find_resource_raw(uint16_t hmod, uint16_t type_id, uint16_t name_id, const uint8_t **data, uint32_t *len)
{
    NEModule *m = module_for(hmod);
    const uint8_t *p = m->raw + m->rsrctab + 2;
    unsigned shift = (unsigned)(m->raw[m->rsrctab] | (m->raw[m->rsrctab + 1] << 8));
    while (p[0] | p[1]) {
        uint16_t tid = (uint16_t)(p[0] | (p[1] << 8));
        uint16_t cnt = (uint16_t)(p[2] | (p[3] << 8));
        const uint8_t *e = p + 8;
        unsigned k;
        for (k = 0; k < cnt; ++k, e += 12) {
            uint16_t id = (uint16_t)(e[6] | (e[7] << 8));
            if (tid == (0x8000u | type_id) && id == (0x8000u | name_id)) {
                *data = m->raw + ((uint32_t)(e[0] | (e[1] << 8)) << shift);
                *len = (uint32_t)(e[2] | (e[3] << 8)) << shift;
                return 0;
            }
        }
        p = e;
    }
    return -1;
}

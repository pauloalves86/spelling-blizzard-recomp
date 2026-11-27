/* Host USER (+KEYBOARD) for the recompiled game, on SDL2.
 *
 * Window manager, message queue, painting, input, cursor and presentation.
 * The SDL window shows the emulated 640x480 Windows screen; game windows are
 * placed on it at their Win16 coordinates.
 */
#include <SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "api.h"
#include "gdi.h"
#include "hostfs.h"
#include "win16.h"

void mm_poll(void);
void mm_init(int enable_audio);
void mm_shutdown(void);

#define DESKTOP_HWND 0xFFFFu
#define MAX_WND 64
#define MAX_CLASS 32

typedef struct WndClass {
    int used;
    char name[64];
    uint16_t style;
    uint32_t wndproc;
    int16_t cls_extra, wnd_extra;
    uint16_t hinst, hicon, hcursor, hbrush;
    uint32_t menu, classname_fp;
    uint16_t atom;
} WndClass;

typedef struct Wnd {
    int used;
    int cls;
    uint32_t wndproc;
    uint32_t style;
    int x, y, w, h; /* window rect relative to parent client (screen for top-level) */
    uint16_t parent;
    uint16_t hinst, hmenu;
    int visible;
    char title[128];
    uint8_t extra[64];
    int has_update, erase;
    int ux0, uy0, ux1, uy1; /* update rect, client coords */
    uint32_t z;
} Wnd;

static WndClass g_cls[MAX_CLASS];
static Wnd g_wnd[MAX_WND];
static uint32_t g_zcounter;
static uint16_t g_focus, g_active;

typedef struct Msg {
    uint16_t hwnd, msg, wp;
    uint32_t lp, time;
    int16_t x, y;
} Msg;
#define QSIZE 1024
static Msg g_q[QSIZE];
static int g_qhead, g_qcount;
static int g_quit_posted;
static uint16_t g_quit_code;

static int16_t g_keys[256];
static int g_mouse_x, g_mouse_y;

static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;
static uint32_t g_argb[SCREEN_W * SCREEN_H];
static Win16Options g_opt;
static uint32_t g_t0;
static uint32_t g_last_present;

/* ------------------------------------------------------------ time */

static uint32_t now_ms(void) { return SDL_GetTicks() - g_t0; }

/* ------------------------------------------------------------ windows */

static Wnd *wnd_get(uint16_t h)
{
    if (h >= 1 && h < MAX_WND && g_wnd[h].used) {
        return &g_wnd[h];
    }
    return NULL;
}

static void client_insets(const Wnd *w, int *l, int *t, int *r, int *b)
{
    int f = 0, cap = 0;
    if (w->style & 0x00040000u) f = 4;       /* WS_THICKFRAME */
    else if (w->style & 0x00800000u) f = 1;  /* WS_BORDER */
    if ((w->style & 0x00C00000u) == 0x00C00000u) cap = 18; /* WS_CAPTION */
    if (w->style & 0x00400000u && !(w->style & 0x00800000u)) f = 4; /* WS_DLGFRAME */
    *l = *r = *b = f;
    *t = f + cap;
}

/* client origin in screen coordinates */
static void client_origin(const Wnd *w, int *sx, int *sy)
{
    int l, t, r, b;
    int px = 0, py = 0;
    client_insets(w, &l, &t, &r, &b);
    if (w->parent && wnd_get(w->parent)) {
        client_origin(wnd_get(w->parent), &px, &py);
    }
    *sx = px + w->x + l;
    *sy = py + w->y + t;
}

static void client_size(const Wnd *w, int *cw, int *ch)
{
    int l, t, r, b;
    client_insets(w, &l, &t, &r, &b);
    *cw = w->w - l - r;
    *ch = w->h - t - b;
    if (*cw < 0) *cw = 0;
    if (*ch < 0) *ch = 0;
}

int user_client_area(uint16_t hwnd, int *x, int *y, int *w, int *h)
{
    Wnd *wd;
    if (hwnd == DESKTOP_HWND) {
        *x = *y = 0;
        *w = SCREEN_W;
        *h = SCREEN_H;
        return 1;
    }
    wd = wnd_get(hwnd);
    if (!wd) {
        return 0;
    }
    client_origin(wd, x, y);
    client_size(wd, w, h);
    return 1;
}

static uint32_t call_wndproc(uint16_t hwnd, uint16_t msg, uint16_t wp, uint32_t lp)
{
    Wnd *w = wnd_get(hwnd);
    if (!w || !w->wndproc) {
        return 0;
    }
    PUSH16(hwnd);
    PUSH16(msg);
    PUSH16(wp);
    PUSH32(lp);
    recomp_callback((uint16_t)(w->wndproc >> 16), (uint16_t)w->wndproc);
    return ((uint32_t)DX << 16) | AX;
}

static uint32_t send_msg(uint16_t hwnd, uint16_t msg, uint16_t wp, uint32_t lp)
{
    uint16_t sax = AX, sbx = BX, scx = CX, sdx = DX;
    uint32_t r = call_wndproc(hwnd, msg, wp, lp);
    (void)sax; (void)sbx; (void)scx; (void)sdx;
    return r;
}

static void post(uint16_t hwnd, uint16_t msg, uint16_t wp, uint32_t lp)
{
    Msg *m;
    if (g_qcount >= QSIZE) {
        return;
    }
    m = &g_q[(g_qhead + g_qcount++) % QSIZE];
    m->hwnd = hwnd;
    m->msg = msg;
    m->wp = wp;
    m->lp = lp;
    m->time = now_ms();
    m->x = (int16_t)g_mouse_x;
    m->y = (int16_t)g_mouse_y;
}

static void invalidate(Wnd *w, int x0, int y0, int x1, int y1, int erase)
{
    int cw, ch;
    client_size(w, &cw, &ch);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > cw) x1 = cw;
    if (y1 > ch) y1 = ch;
    if (x1 <= x0 || y1 <= y0) {
        return;
    }
    if (!w->has_update) {
        w->ux0 = x0; w->uy0 = y0; w->ux1 = x1; w->uy1 = y1;
        w->has_update = 1;
    } else {
        if (x0 < w->ux0) w->ux0 = x0;
        if (y0 < w->uy0) w->uy0 = y0;
        if (x1 > w->ux1) w->ux1 = x1;
        if (y1 > w->uy1) w->uy1 = y1;
    }
    if (erase) {
        w->erase = 1;
    }
}

static uint16_t hit_test(int sx, int sy)
{
    uint16_t best = 0;
    uint32_t bz = 0;
    int i;
    for (i = 1; i < MAX_WND; ++i) {
        Wnd *w = &g_wnd[i];
        int x, y, cw, ch;
        if (!w->used || !w->visible) continue;
        if (w->parent && wnd_get(w->parent) && !wnd_get(w->parent)->visible) continue;
        client_origin(w, &x, &y);
        client_size(w, &cw, &ch);
        if (sx >= x && sy >= y && sx < x + cw && sy < y + ch && w->z >= bz) {
            best = (uint16_t)i;
            bz = w->z;
        }
    }
    return best;
}

/* ------------------------------------------------------------ cursors */

typedef struct Cursor {
    int used;
    int w, h, hx, hy;
    uint8_t and_m[32 * 32], xor_m[32 * 32];
} Cursor;
#define MAX_CURSOR 32
static Cursor g_cur[MAX_CURSOR];
static uint16_t g_cursor;
static int g_cursor_count = 0;
#define CUR_BASE 0x3000u

static uint16_t cursor_builtin(int kind)
{
    static const char *arrow[] = {
        "X           ", "XX          ", "X.X         ", "X..X        ", "X...X       ", "X....X      ",
        "X.....X     ", "X......X    ", "X.......X   ", "X........X  ", "X.....XXXXX ", "X..X..X     ",
        "X.X X..X    ", "XX  X..X    ", "X    X..X   ", "     X..X   ", "      X..X  ", "      X..X  ",
        "       XX   ", NULL};
    static const char *wait[] = {
        "XXXXXXXXXXX", "X.........X", " X.......X ", " X.X.X.X.X ", "  X.X.X.X  ", "   X.X.X   ",
        "    X.X    ", "    X.X    ", "   X...X   ", "  X.....X  ", " X...X...X ", " X..X.X..X ",
        "X.X.X.X.X.X", "XXXXXXXXXXX", NULL};
    const char **art = kind == 1 ? wait : arrow;
    int i, x, y;
    for (i = 1; i < MAX_CURSOR; ++i) {
        if (!g_cur[i].used) break;
    }
    if (i == MAX_CURSOR) return 0;
    memset(&g_cur[i], 0, sizeof(Cursor));
    g_cur[i].used = 1;
    g_cur[i].w = g_cur[i].h = 32;
    memset(g_cur[i].and_m, 1, sizeof(g_cur[i].and_m));
    for (y = 0; art[y]; ++y) {
        for (x = 0; art[y][x]; ++x) {
            if (art[y][x] == 'X') { g_cur[i].and_m[y * 32 + x] = 0; g_cur[i].xor_m[y * 32 + x] = 0; }
            if (art[y][x] == '.') { g_cur[i].and_m[y * 32 + x] = 0; g_cur[i].xor_m[y * 32 + x] = 1; }
        }
    }
    if (kind == 1) { g_cur[i].hx = 5; g_cur[i].hy = 7; }
    return (uint16_t)(CUR_BASE + i);
}

static uint16_t cursor_from_resource(const uint8_t *d, uint32_t len)
{
    int i, x, y, w, h, stride, bc, ncol;
    const uint8_t *bih;
    const uint8_t *xorb, *andb;
    if (len < 4 + 40) return 0;
    for (i = 1; i < MAX_CURSOR; ++i) {
        if (!g_cur[i].used) break;
    }
    if (i == MAX_CURSOR) return 0;
    memset(&g_cur[i], 0, sizeof(Cursor));
    g_cur[i].used = 1;
    g_cur[i].hx = d[0] | (d[1] << 8);
    g_cur[i].hy = d[2] | (d[3] << 8);
    bih = d + 4;
    w = bih[4] | (bih[5] << 8);
    h = (bih[8] | (bih[9] << 8)) / 2;
    bc = bih[14] | (bih[15] << 8);
    if (w > 32) w = 32;
    if (h > 32) h = 32;
    g_cur[i].w = w;
    g_cur[i].h = h;
    ncol = bc <= 8 ? (1 << bc) : 0;
    stride = ((w * bc + 31) / 32) * 4;
    xorb = bih + 40 + 4 * ncol;
    andb = xorb + stride * h;
    for (y = 0; y < h; ++y) {
        int ry = h - 1 - y;
        for (x = 0; x < w; ++x) {
            int astride = ((w + 31) / 32) * 4;
            g_cur[i].and_m[y * 32 + x] = (uint8_t)((andb[ry * astride + (x >> 3)] >> (7 - (x & 7))) & 1);
            g_cur[i].xor_m[y * 32 + x] = (uint8_t)((xorb[ry * stride + (x >> 3)] >> (7 - (x & 7))) & 1);
        }
    }
    return (uint16_t)(CUR_BASE + i);
}

/* ------------------------------------------------------------ presentation */

static void present(int force)
{
    int i, x, y;
    uint32_t lut[256];
    uint32_t t = now_ms();
    if (!force && t - g_last_present < 15) {
        return;
    }
    g_last_present = t;
    for (i = 0; i < 256; ++i) {
        lut[i] = 0xFF000000u | ((uint32_t)g_syspal[i].r << 16) | ((uint32_t)g_syspal[i].g << 8) | g_syspal[i].b;
    }
    for (i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        g_argb[i] = lut[g_screen[i]];
    }
    g_screen_dirty = 0;
    /* software cursor */
    if (g_cursor >= CUR_BASE && g_cursor < CUR_BASE + MAX_CURSOR && g_cursor_count >= 0) {
        Cursor *c = &g_cur[g_cursor - CUR_BASE];
        for (y = 0; y < c->h; ++y) {
            for (x = 0; x < c->w; ++x) {
                int sx = g_mouse_x - c->hx + x, sy = g_mouse_y - c->hy + y;
                uint32_t *p;
                if (sx < 0 || sy < 0 || sx >= SCREEN_W || sy >= SCREEN_H) continue;
                p = &g_argb[sy * SCREEN_W + sx];
                if (!c->and_m[y * 32 + x]) {
                    *p = c->xor_m[y * 32 + x] ? 0xFFFFFFFFu : 0xFF000000u;
                } else if (c->xor_m[y * 32 + x]) {
                    *p ^= 0x00FFFFFFu;
                }
            }
        }
    }
    if (g_tex) {
        SDL_UpdateTexture(g_tex, NULL, g_argb, SCREEN_W * 4);
        SDL_RenderClear(g_ren);
        SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
        SDL_RenderPresent(g_ren);
    }
}

static void write_snapshot(const char *path)
{
    FILE *fp = fopen(path, "wb");
    int i;
    present(1);
    if (!fp) return;
    fprintf(fp, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (i = 0; i < SCREEN_W * SCREEN_H; ++i) {
        uint8_t rgb[3] = {(uint8_t)(g_argb[i] >> 16), (uint8_t)(g_argb[i] >> 8), (uint8_t)g_argb[i]};
        fwrite(rgb, 1, 3, fp);
    }
    fclose(fp);
    fprintf(stderr, "[recomp] snapshot %s at %u ms\n", path, now_ms());
}

/* ------------------------------------------------------------ input */

static int vk_from_sdl(SDL_Keycode k)
{
    if (k >= SDLK_a && k <= SDLK_z) return 'A' + (int)(k - SDLK_a);
    if (k >= SDLK_0 && k <= SDLK_9) return '0' + (int)(k - SDLK_0);
    if (k >= SDLK_F1 && k <= SDLK_F12) return 0x70 + (int)(k - SDLK_F1);
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9) return 0x61 + (int)(k - SDLK_KP_1);
    switch (k) {
        case SDLK_KP_0: return 0x60;
        case SDLK_BACKSPACE: return 0x08;
        case SDLK_TAB: return 0x09;
        case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
        case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
        case SDLK_LCTRL: case SDLK_RCTRL: return 0x11;
        case SDLK_LALT: case SDLK_RALT: return 0x12;
        case SDLK_PAUSE: return 0x13;
        case SDLK_CAPSLOCK: return 0x14;
        case SDLK_ESCAPE: return 0x1B;
        case SDLK_SPACE: return 0x20;
        case SDLK_PAGEUP: return 0x21;
        case SDLK_PAGEDOWN: return 0x22;
        case SDLK_END: return 0x23;
        case SDLK_HOME: return 0x24;
        case SDLK_LEFT: return 0x25;
        case SDLK_UP: return 0x26;
        case SDLK_RIGHT: return 0x27;
        case SDLK_DOWN: return 0x28;
        case SDLK_INSERT: return 0x2D;
        case SDLK_DELETE: return 0x2E;
        case SDLK_MINUS: return 0xBD;
        case SDLK_EQUALS: return 0xBB;
        case SDLK_COMMA: return 0xBC;
        case SDLK_PERIOD: return 0xBE;
        case SDLK_SLASH: return 0xBF;
        case SDLK_SEMICOLON: return 0xBA;
        case SDLK_QUOTE: return 0xDE;
        case SDLK_LEFTBRACKET: return 0xDB;
        case SDLK_RIGHTBRACKET: return 0xDD;
        case SDLK_BACKSLASH: return 0xDC;
        case SDLK_BACKQUOTE: return 0xC0;
        default: return 0;
    }
}

static uint16_t focus_target(void)
{
    if (wnd_get(g_focus)) return g_focus;
    if (wnd_get(g_active)) return g_active;
    return 0;
}

static void key_event(int vk, int down, int repeat)
{
    uint16_t hwnd = focus_target();
    uint32_t lp = 1u | ((uint32_t)(vk & 0xFF) << 16);
    int sys = (g_keys[0x12] & 0x8000) && vk != 0x12;
    if (!down) lp |= 0xC0000000u;
    else if (repeat) lp |= 0x40000000u;
    if (down) {
        if (!(g_keys[vk] & 0x8000)) g_keys[vk] ^= 1; /* toggle bit */
        g_keys[vk] |= (int16_t)0x8000;
    } else {
        g_keys[vk] &= 1;
    }
    if (!hwnd) return;
    if (sys || vk == 0x12) {
        post(hwnd, down ? 0x104 : 0x105, (uint16_t)vk, lp | (sys ? 0x20000000u : 0));
    } else {
        post(hwnd, down ? 0x100 : 0x101, (uint16_t)vk, lp);
    }
}

static void mouse_event(uint16_t msg, int x, int y)
{
    uint16_t hwnd = hit_test(x, y);
    Wnd *w = wnd_get(hwnd);
    int cx, cy;
    uint16_t flags = 0;
    if (!w) return;
    client_origin(w, &cx, &cy);
    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK) flags |= 1;
    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_RMASK) flags |= 2;
    if (g_keys[0x10] & 0x8000) flags |= 4;
    if (g_keys[0x11] & 0x8000) flags |= 8;
    if (msg == 0x200) {
        /* WM_SETCURSOR is sent synchronously before the mouse move */
        if (send_msg(hwnd, 0x20, hwnd, ((uint32_t)0x200 << 16) | 1) == 0) {
            /* not handled: DefWindowProc behaviour already applied */
        }
    }
    if (msg == 0x203 && !(g_cls[w->cls].style & 0x8)) {
        msg = 0x201;
    }
    if (msg == 0x206 && !(g_cls[w->cls].style & 0x8)) {
        msg = 0x204;
    }
    post(hwnd, msg, flags, ((uint32_t)(uint16_t)(y - cy) << 16) | (uint16_t)(x - cx));
}

/* scripted input for automated runs */
typedef struct ScriptEv { uint32_t t; char cmd[16]; char arg[128]; } ScriptEv;
static ScriptEv *g_script;
static int g_script_n, g_script_i;

static void load_script(const char *path)
{
    FILE *fp = fopen(path, "r");
    char line[256];
    if (!fp) {
        fprintf(stderr, "cannot open script %s\n", path);
        exit(2);
    }
    while (fgets(line, sizeof(line), fp)) {
        ScriptEv e;
        char *p = line;
        memset(&e, 0, sizeof(e));
        line[strcspn(line, "\r\n")] = 0;
        while (isspace((unsigned char)*p)) ++p;
        if (!*p || *p == '#') continue;
        if (sscanf(p, "%u %15s %127[^\n]", &e.t, e.cmd, e.arg) < 2) continue;
        g_script = (ScriptEv *)realloc(g_script, sizeof(ScriptEv) * (size_t)(g_script_n + 1));
        g_script[g_script_n++] = e;
    }
    fclose(fp);
}

static int key_by_name(const char *n)
{
    static const struct { const char *n; int vk; } t[] = {
        {"RETURN", 0x0D}, {"ENTER", 0x0D}, {"ESC", 0x1B}, {"ESCAPE", 0x1B}, {"SPACE", 0x20}, {"BACK", 0x08},
        {"TAB", 0x09}, {"LEFT", 0x25}, {"UP", 0x26}, {"RIGHT", 0x27}, {"DOWN", 0x28}, {"F1", 0x70}, {"F2", 0x71},
        {"F3", 0x72}, {"F4", 0x73}, {"SHIFT", 0x10}, {"CTRL", 0x11}, {"ALT", 0x12}, {NULL, 0}};
    int i;
    for (i = 0; t[i].n; ++i) {
        if (!strcasecmp(n, t[i].n)) return t[i].vk;
    }
    if (strlen(n) == 1) return toupper((unsigned char)n[0]);
    return 0;
}

static void run_script(void)
{
    uint32_t t = now_ms();
    while (g_script_i < g_script_n && g_script[g_script_i].t <= t) {
        ScriptEv *e = &g_script[g_script_i++];
        if (!strcmp(e->cmd, "key")) {
            int vk = key_by_name(e->arg);
            key_event(vk, 1, 0);
            key_event(vk, 0, 0);
        } else if (!strcmp(e->cmd, "down") || !strcmp(e->cmd, "up")) {
            key_event(key_by_name(e->arg), e->cmd[0] == 'd', 0);
        } else if (!strcmp(e->cmd, "text")) {
            const char *s;
            for (s = e->arg; *s; ++s) {
                int vk = toupper((unsigned char)*s);
                if (*s == ' ') vk = 0x20;
                key_event(vk, 1, 0);
                key_event(vk, 0, 0);
            }
        } else if (!strcmp(e->cmd, "move") || !strcmp(e->cmd, "click") || !strcmp(e->cmd, "rclick")) {
            int x = 0, y = 0;
            sscanf(e->arg, "%d %d", &x, &y);
            g_mouse_x = x;
            g_mouse_y = y;
            mouse_event(0x200, x, y);
            if (e->cmd[0] == 'c') {
                mouse_event(0x201, x, y);
                mouse_event(0x202, x, y);
            } else if (e->cmd[0] == 'r') {
                mouse_event(0x204, x, y);
                mouse_event(0x205, x, y);
            }
        } else if (!strcmp(e->cmd, "snap")) {
            write_snapshot(e->arg);
        } else if (!strcmp(e->cmd, "quit")) {
            fprintf(stderr, "[recomp] script quit at %u ms\n", t);
            win16_shutdown();
            exit(0);
        }
    }
}

static void pump_host(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                if (wnd_get(g_active)) post(g_active, 0x10, 0, 0); /* WM_CLOSE */
                else exit(0);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                int vk = vk_from_sdl(ev.key.keysym.sym);
                if (vk) key_event(vk, ev.type == SDL_KEYDOWN, ev.key.repeat);
                break;
            }
            case SDL_MOUSEMOTION:
                g_mouse_x = ev.motion.x;
                g_mouse_y = ev.motion.y;
                mouse_event(0x200, g_mouse_x, g_mouse_y);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                int down = ev.type == SDL_MOUSEBUTTONDOWN;
                uint16_t m;
                g_mouse_x = ev.button.x;
                g_mouse_y = ev.button.y;
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    g_keys[0x01] = down ? (int16_t)0x8000 : 0;
                    m = down ? (ev.button.clicks == 2 ? 0x203 : 0x201) : 0x202;
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    g_keys[0x02] = down ? (int16_t)0x8000 : 0;
                    m = down ? (ev.button.clicks == 2 ? 0x206 : 0x204) : 0x205;
                } else {
                    break;
                }
                mouse_event(m, g_mouse_x, g_mouse_y);
                break;
            }
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_EXPOSED) g_screen_dirty = 1;
                break;
            default:
                break;
        }
    }
    if (g_script) run_script();
    if (g_opt.max_ms && now_ms() >= g_opt.max_ms) {
        if (g_opt.snapshot) write_snapshot(g_opt.snapshot);
        fprintf(stderr, "[recomp] --quit-after reached at %u ms\n", now_ms());
        win16_shutdown();
        exit(0);
    }
}

/* Called whenever the game polls (PeekMessage, timeGetTime). */
void user_idle(void)
{
    static uint32_t last;
    uint32_t t = now_ms();
    if (t != last) {
        last = t;
        pump_host();
        mm_poll();
    }
    present(0);
}

/* ------------------------------------------------------------ init */

int win16_init(const Win16Options *opt)
{
    const char *font_dir = hostfs_font_dir();
    g_opt = *opt;
    if (opt->headless) {
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | (opt->audio ? SDL_INIT_AUDIO : 0)) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    g_t0 = SDL_GetTicks();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    g_win = SDL_CreateWindow("Spelling Blizzard", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_W * (opt->scale > 0 ? opt->scale : 2), SCREEN_H * (opt->scale > 0 ? opt->scale : 2),
                             SDL_WINDOW_RESIZABLE | (opt->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (!g_win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return -1;
    }
    g_ren = SDL_CreateRenderer(g_win, -1, 0);
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_SOFTWARE);
    SDL_RenderSetLogicalSize(g_ren, SCREEN_W, SCREEN_H);
    g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    SDL_ShowCursor(SDL_DISABLE);
    gdi_init(font_dir);
    mm_init(opt->audio);
    memset(g_wnd, 0, sizeof(g_wnd));
    memset(g_cls, 0, sizeof(g_cls));
    g_cursor = cursor_builtin(0);
    if (opt->script) load_script(opt->script);
    return 0;
}

void win16_shutdown(void)
{
    static int done;
    if (done) return;
    done = 1;
    if (g_opt.snapshot && !g_opt.max_ms) write_snapshot(g_opt.snapshot);
    mm_shutdown();
    SDL_Quit();
}

/* ============================================================ API */

void api_MMSYSTEM_TIMEGETTIME(void)
{
    static uint32_t last_idle;
    uint32_t t = now_ms();
    if (t - last_idle >= 5) {
        last_idle = t;
        user_idle();
    }
    ret_d(t + 10000u);
    api_ret(0);
}

void api_USER_REGISTERCLASS(void)
{
    ArgReader a = args_pascal(4);
    uint8_t *wc = (uint8_t *)farp(arg_d(&a));
    int i;
    uint32_t namefp;
    for (i = 1; i < MAX_CLASS && g_cls[i].used; ++i) {
    }
    AX = 0;
    if (i < MAX_CLASS && wc) {
        WndClass *c = &g_cls[i];
        memset(c, 0, sizeof(*c));
        c->used = 1;
        memcpy(&c->style, wc, 2);
        memcpy(&c->wndproc, wc + 2, 4);
        memcpy(&c->cls_extra, wc + 6, 2);
        memcpy(&c->wnd_extra, wc + 8, 2);
        memcpy(&c->hinst, wc + 10, 2);
        memcpy(&c->hicon, wc + 12, 2);
        memcpy(&c->hcursor, wc + 14, 2);
        memcpy(&c->hbrush, wc + 16, 2);
        memcpy(&c->menu, wc + 18, 4);
        memcpy(&namefp, wc + 22, 4);
        c->classname_fp = namefp;
        far_to_c(namefp, c->name, sizeof(c->name));
        c->atom = (uint16_t)(0xC000 + i);
        AX = c->atom;
        api_log("RegisterClass(%s) wndproc %08x", c->name, c->wndproc);
    }
    api_ret(4);
}

static int find_class(uint32_t namefp)
{
    char n[64];
    int i;
    if ((namefp >> 16) == 0) {
        for (i = 1; i < MAX_CLASS; ++i) {
            if (g_cls[i].used && g_cls[i].atom == (uint16_t)namefp) return i;
        }
        return 0;
    }
    far_to_c(namefp, n, sizeof(n));
    for (i = 1; i < MAX_CLASS; ++i) {
        if (g_cls[i].used && !strcasecmp(g_cls[i].name, n)) return i;
    }
    return 0;
}

void api_USER_GETCLASSINFO(void)
{
    ArgReader a = args_pascal(10);
    uint16_t hinst = arg_w(&a);
    uint32_t name = arg_d(&a);
    uint8_t *wc = (uint8_t *)farp(arg_d(&a));
    int c = find_class(name);
    (void)hinst;
    AX = 0;
    if (c && wc) {
        WndClass *k = &g_cls[c];
        memcpy(wc, &k->style, 2);
        memcpy(wc + 2, &k->wndproc, 4);
        memcpy(wc + 6, &k->cls_extra, 2);
        memcpy(wc + 8, &k->wnd_extra, 2);
        memcpy(wc + 10, &k->hinst, 2);
        memcpy(wc + 12, &k->hicon, 2);
        memcpy(wc + 14, &k->hcursor, 2);
        memcpy(wc + 16, &k->hbrush, 2);
        memcpy(wc + 18, &k->menu, 4);
        memcpy(wc + 22, &k->classname_fp, 4);
        AX = 1;
    }
    api_ret(10);
}

void api_USER_GETCLASSWORD(void)
{
    ArgReader a = args_pascal(4);
    Wnd *w = wnd_get(arg_w(&a));
    int16_t idx = arg_i(&a);
    AX = 0;
    if (w) {
        WndClass *c = &g_cls[w->cls];
        switch (idx) {
            case -10: AX = c->hbrush; break;
            case -12: AX = c->hcursor; break;
            case -14: AX = c->hicon; break;
            case -16: AX = c->hinst; break;
            case -18: AX = (uint16_t)c->wnd_extra; break;
            case -20: AX = (uint16_t)c->cls_extra; break;
            case -26: AX = c->style; break;
            default: break;
        }
    }
    api_ret(4);
}

void api_USER_CREATEWINDOW(void)
{
    ArgReader a = args_pascal(30);
    uint32_t clsname = arg_d(&a), title = arg_d(&a), style = arg_d(&a);
    int16_t x = arg_i(&a), y = arg_i(&a), w = arg_i(&a), h = arg_i(&a);
    uint16_t parent = arg_w(&a), menu = arg_w(&a), hinst = arg_w(&a);
    uint32_t param = arg_d(&a);
    int c = find_class(clsname);
    int i;
    Wnd *wd;
    uint16_t hwnd;
    uint16_t saved_sp;
    uint32_t r;
    api_ret(30);
    if (!c) {
        char n[64];
        far_to_c(clsname, n, sizeof(n));
        fprintf(stderr, "[recomp] CreateWindow: unknown class '%s'\n", n);
        AX = 0;
        return;
    }
    for (i = 1; i < MAX_WND && g_wnd[i].used; ++i) {
    }
    if (i == MAX_WND) {
        AX = 0;
        return;
    }
    hwnd = (uint16_t)i;
    wd = &g_wnd[i];
    memset(wd, 0, sizeof(*wd));
    wd->used = 1;
    wd->cls = c;
    wd->wndproc = g_cls[c].wndproc;
    wd->style = style;
    if (x == (int16_t)0x8000) { x = 0; y = 0; }
    if (w == (int16_t)0x8000) { w = SCREEN_W; h = SCREEN_H; }
    wd->x = x; wd->y = y; wd->w = w; wd->h = h;
    wd->parent = parent;
    wd->hinst = hinst;
    wd->hmenu = menu;
    wd->z = ++g_zcounter;
    far_to_c(title, wd->title, sizeof(wd->title));
    api_log("CreateWindow(%s, '%s', style %08x, %d,%d %dx%d, parent %04x) = %04x",
            g_cls[c].name, wd->title, style, x, y, w, h, parent, hwnd);

    /* CREATESTRUCT on the emulated stack */
    saved_sp = SP;
    SP -= 32;
    {
        uint16_t cs_off = SP;
        WD(SS, cs_off, param);
        WW(SS, (uint16_t)(cs_off + 4), hinst);
        WW(SS, (uint16_t)(cs_off + 6), menu);
        WW(SS, (uint16_t)(cs_off + 8), parent);
        WW(SS, (uint16_t)(cs_off + 10), (uint16_t)h);
        WW(SS, (uint16_t)(cs_off + 12), (uint16_t)w);
        WW(SS, (uint16_t)(cs_off + 14), (uint16_t)y);
        WW(SS, (uint16_t)(cs_off + 16), (uint16_t)x);
        WD(SS, (uint16_t)(cs_off + 18), style);
        WD(SS, (uint16_t)(cs_off + 22), title);
        WD(SS, (uint16_t)(cs_off + 26), clsname);
        WD(SS, (uint16_t)(cs_off + 30 - 2), 0);
        r = send_msg(hwnd, 0x81, 0, mkfar(SS, cs_off)); /* WM_NCCREATE */
        if (r) {
            r = send_msg(hwnd, 0x01, 0, mkfar(SS, cs_off)); /* WM_CREATE */
        } else {
            r = 0xFFFFFFFFu;
        }
    }
    SP = saved_sp;
    if (r == 0xFFFFFFFFu) {
        wd->used = 0;
        AX = 0;
        return;
    }
    {
        int cw, ch;
        client_size(wd, &cw, &ch);
        send_msg(hwnd, 0x05, 0, ((uint32_t)(uint16_t)ch << 16) | (uint16_t)cw); /* WM_SIZE */
        send_msg(hwnd, 0x03, 0, ((uint32_t)(uint16_t)wd->y << 16) | (uint16_t)wd->x); /* WM_MOVE */
    }
    if (style & 0x10000000u) { /* WS_VISIBLE */
        wd->visible = 1;
        invalidate(wd, 0, 0, 32767, 32767, 1);
    }
    AX = hwnd;
}

void api_USER_SHOWWINDOW(void)
{
    ArgReader a = args_pascal(4);
    uint16_t hwnd = arg_w(&a);
    int16_t cmd = arg_i(&a);
    Wnd *w = wnd_get(hwnd);
    api_ret(4);
    AX = 0;
    if (!w) return;
    AX = (uint16_t)w->visible;
    if (cmd == 0) {
        if (w->visible) {
            w->visible = 0;
            send_msg(hwnd, 0x18, 0, 0); /* WM_SHOWWINDOW FALSE */
        }
    } else {
        if (!w->visible) {
            w->visible = 1;
            send_msg(hwnd, 0x18, 1, 0);
            invalidate(w, 0, 0, 32767, 32767, 1);
        }
        if (!w->parent && g_active != hwnd) {
            uint16_t old = g_active;
            g_active = hwnd;
            send_msg(hwnd, 0x1C, 1, 0);              /* WM_ACTIVATEAPP */
            send_msg(hwnd, 0x86, 1, 0);              /* WM_NCACTIVATE */
            send_msg(hwnd, 0x06, 1, old);            /* WM_ACTIVATE WA_ACTIVE */
            g_focus = hwnd;
            send_msg(hwnd, 0x07, old, 0);            /* WM_SETFOCUS */
            send_msg(hwnd, 0x0F + 0x300, 0, 0);      /* WM_QUERYNEWPALETTE (0x30F) */
        }
    }
}

void api_USER_DESTROYWINDOW(void)
{
    ArgReader a = args_pascal(2);
    uint16_t hwnd = arg_w(&a);
    Wnd *w = wnd_get(hwnd);
    int i;
    api_ret(2);
    AX = 0;
    if (!w) return;
    for (i = 1; i < MAX_WND; ++i) {
        if (g_wnd[i].used && g_wnd[i].parent == hwnd) {
            PUSH16((uint16_t)i);
            PUSH16(0); PUSH16(0); /* fake return address */
            api_USER_DESTROYWINDOW();
        }
    }
    if (w->visible) {
        w->visible = 0;
    }
    send_msg(hwnd, 0x02, 0, 0);  /* WM_DESTROY */
    send_msg(hwnd, 0x82, 0, 0);  /* WM_NCDESTROY */
    w->used = 0;
    if (g_focus == hwnd) g_focus = 0;
    if (g_active == hwnd) g_active = 0;
    AX = 1;
}

void api_USER_GETWINDOW(void)
{
    ArgReader a = args_pascal(4);
    uint16_t hwnd = arg_w(&a);
    uint16_t cmd = arg_w(&a);
    int i;
    AX = 0;
    if (cmd == 5) { /* GW_CHILD */
        for (i = 1; i < MAX_WND; ++i) {
            if (g_wnd[i].used && g_wnd[i].parent == hwnd) { AX = (uint16_t)i; break; }
        }
    } else if (cmd == 2) { /* GW_HWNDNEXT */
        Wnd *w = wnd_get(hwnd);
        if (w) {
            for (i = hwnd + 1; i < MAX_WND; ++i) {
                if (g_wnd[i].used && g_wnd[i].parent == w->parent) { AX = (uint16_t)i; break; }
            }
        }
    } else if (cmd == 4) { /* GW_OWNER */
        AX = 0;
    }
    api_ret(4);
}

void api_USER_GETWINDOWLONG(void)
{
    ArgReader a = args_pascal(4);
    Wnd *w = wnd_get(arg_w(&a));
    int16_t idx = arg_i(&a);
    uint32_t v = 0;
    if (w) {
        if (idx >= 0 && idx + 4 <= 64) memcpy(&v, w->extra + idx, 4);
        else if (idx == -4) v = w->wndproc;
        else if (idx == -16) v = w->style;
        else if (idx == -20) v = 0;
    }
    ret_d(v);
    api_ret(4);
}

void api_USER_SETWINDOWLONG(void)
{
    ArgReader a = args_pascal(8);
    Wnd *w = wnd_get(arg_w(&a));
    int16_t idx = arg_i(&a);
    uint32_t v = arg_d(&a), old = 0;
    if (w) {
        if (idx >= 0 && idx + 4 <= 64) { memcpy(&old, w->extra + idx, 4); memcpy(w->extra + idx, &v, 4); }
        else if (idx == -4) { old = w->wndproc; w->wndproc = v; }
        else if (idx == -16) { old = w->style; w->style = v; }
    }
    ret_d(old);
    api_ret(8);
}

void api_USER_GETWINDOWTEXT(void)
{
    ArgReader a = args_pascal(8);
    Wnd *w = wnd_get(arg_w(&a));
    uint32_t buf = arg_d(&a);
    int16_t n = arg_i(&a);
    AX = 0;
    if (w && n > 0) {
        c_to_far(w->title, buf, (size_t)n);
        AX = (uint16_t)strlen(w->title);
    }
    api_ret(8);
}

static void put_rect(uint32_t fp, int l, int t, int r, int b)
{
    int16_t v[4] = {(int16_t)l, (int16_t)t, (int16_t)r, (int16_t)b};
    if (fp) memcpy(farp(fp), v, 8);
}

void api_USER_GETCLIENTRECT(void)
{
    ArgReader a = args_pascal(6);
    Wnd *w = wnd_get(arg_w(&a));
    uint32_t rc = arg_d(&a);
    int cw = SCREEN_W, ch = SCREEN_H;
    if (w) client_size(w, &cw, &ch);
    put_rect(rc, 0, 0, cw, ch);
    api_ret(6);
}

void api_USER_GETWINDOWRECT(void)
{
    ArgReader a = args_pascal(6);
    uint16_t hwnd = arg_w(&a);
    Wnd *w = wnd_get(hwnd);
    uint32_t rc = arg_d(&a);
    if (w) {
        int px = 0, py = 0;
        if (w->parent && wnd_get(w->parent)) client_origin(wnd_get(w->parent), &px, &py);
        put_rect(rc, px + w->x, py + w->y, px + w->x + w->w, py + w->y + w->h);
    } else {
        put_rect(rc, 0, 0, SCREEN_W, SCREEN_H);
    }
    api_ret(6);
}

void api_USER_CLIENTTOSCREEN(void)
{
    ArgReader a = args_pascal(6);
    Wnd *w = wnd_get(arg_w(&a));
    int16_t *p = (int16_t *)farp(arg_d(&a));
    if (w && p) {
        int x, y;
        client_origin(w, &x, &y);
        p[0] = (int16_t)(p[0] + x);
        p[1] = (int16_t)(p[1] + y);
    }
    api_ret(6);
}

void api_USER_SCREENTOCLIENT(void)
{
    ArgReader a = args_pascal(6);
    Wnd *w = wnd_get(arg_w(&a));
    int16_t *p = (int16_t *)farp(arg_d(&a));
    if (w && p) {
        int x, y;
        client_origin(w, &x, &y);
        p[0] = (int16_t)(p[0] - x);
        p[1] = (int16_t)(p[1] - y);
    }
    api_ret(6);
}

void api_USER_GETDC(void)
{
    ArgReader a = args_pascal(2);
    uint16_t hwnd = arg_w(&a);
    AX = gdi_create_window_dc(hwnd ? hwnd : DESKTOP_HWND);
    api_ret(2);
}

void api_USER_RELEASEDC(void)
{
    ArgReader a = args_pascal(4);
    (void)arg_w(&a);
    gdi_release_window_dc(arg_w(&a));
    AX = 1;
    api_ret(4);
}

void api_USER_INVALIDATERECT(void)
{
    ArgReader a = args_pascal(8);
    uint16_t hwnd = arg_w(&a);
    uint32_t rc = arg_d(&a);
    int16_t erase = arg_i(&a);
    Wnd *w = wnd_get(hwnd);
    if (w) {
        if (rc) {
            int16_t r[4];
            memcpy(r, farp(rc), 8);
            invalidate(w, r[0], r[1], r[2], r[3], erase);
        } else {
            invalidate(w, 0, 0, 32767, 32767, erase);
        }
    }
    api_ret(8);
}

void api_USER_VALIDATERECT(void)
{
    ArgReader a = args_pascal(6);
    Wnd *w = wnd_get(arg_w(&a));
    (void)arg_d(&a);
    if (w) w->has_update = 0;
    api_ret(6);
}

void api_USER_GETUPDATERECT(void)
{
    ArgReader a = args_pascal(8);
    Wnd *w = wnd_get(arg_w(&a));
    uint32_t rc = arg_d(&a);
    (void)arg_w(&a);
    AX = 0;
    if (w && w->has_update) {
        put_rect(rc, w->ux0, w->uy0, w->ux1, w->uy1);
        AX = 1;
    } else {
        put_rect(rc, 0, 0, 0, 0);
    }
    api_ret(8);
}

void api_USER_BEGINPAINT(void)
{
    ArgReader a = args_pascal(6);
    uint16_t hwnd = arg_w(&a);
    uint32_t ps = arg_d(&a);
    Wnd *w = wnd_get(hwnd);
    uint16_t hdc = gdi_create_window_dc(hwnd);
    uint8_t *p = (uint8_t *)farp(ps);
    int erase = 0;
    api_ret(6);
    memset(p, 0, 32);
    memcpy(p, &hdc, 2);
    if (w) {
        int16_t r[4] = {(int16_t)w->ux0, (int16_t)w->uy0, (int16_t)w->ux1, (int16_t)w->uy1};
        if (!w->has_update) memset(r, 0, sizeof(r));
        memcpy(p + 4, r, 8);
        if (w->erase) {
            erase = send_msg(hwnd, 0x14, hdc, 0) == 0; /* WM_ERASEBKGND */
            w->erase = 0;
        }
        w->has_update = 0;
    }
    {
        uint16_t fe = (uint16_t)erase;
        memcpy(p + 2, &fe, 2);
    }
    AX = hdc;
}

void api_USER_ENDPAINT(void)
{
    ArgReader a = args_pascal(6);
    (void)arg_w(&a);
    uint32_t ps = arg_d(&a);
    uint16_t hdc;
    memcpy(&hdc, farp(ps), 2);
    gdi_release_window_dc(hdc);
    AX = 1;
    api_ret(6);
}

void api_USER_DEFWINDOWPROC(void)
{
    ArgReader a = args_pascal(10);
    uint16_t hwnd = arg_w(&a), msg = arg_w(&a), wp = arg_w(&a);
    uint32_t lp = arg_d(&a);
    Wnd *w = wnd_get(hwnd);
    uint32_t r = 0;
    (void)lp;
    api_ret(10);
    switch (msg) {
        case 0x81: r = 1; break; /* WM_NCCREATE */
        case 0x10: /* WM_CLOSE */
            PUSH16(hwnd); PUSH16(0); PUSH16(0);
            api_USER_DESTROYWINDOW();
            break;
        case 0x14: /* WM_ERASEBKGND */
            if (w && g_cls[w->cls].hbrush) {
                int x, y, cw, ch;
                client_origin(w, &x, &y);
                client_size(w, &cw, &ch);
                gdi_fill_screen_rect(x, y, x + cw, y + ch, g_cls[w->cls].hbrush, 1);
                r = 1;
            }
            break;
        case 0x0F: /* WM_PAINT */
            if (w) w->has_update = 0;
            break;
        case 0x20: /* WM_SETCURSOR */
            if (w && (uint16_t)lp == 1 && g_cls[w->cls].hcursor) {
                g_cursor = g_cls[w->cls].hcursor;
                r = 1;
            }
            break;
        case 0x84: r = 1; break; /* WM_NCHITTEST -> HTCLIENT */
        case 0x104:              /* WM_SYSKEYDOWN: Alt+F4 */
            if (wp == 0x73 && w) post(hwnd, 0x10, 0, 0);
            break;
        case 0x86: r = 1; break; /* WM_NCACTIVATE */
        default: break;
    }
    ret_d(r);
}

void api_USER_PEEKMESSAGE(void)
{
    ArgReader a = args_pascal(12);
    uint32_t lpmsg = arg_d(&a);
    uint16_t hwnd = arg_w(&a), mn = arg_w(&a), mx = arg_w(&a), flags = arg_w(&a);
    int i, found = -1;
    Msg m;
    api_ret(12);
    user_idle();
    for (i = 0; i < g_qcount; ++i) {
        Msg *q = &g_q[(g_qhead + i) % QSIZE];
        if (hwnd && q->hwnd != hwnd) continue;
        if ((mn || mx) && (q->msg < mn || q->msg > mx)) continue;
        found = i;
        break;
    }
    if (found >= 0) {
        m = g_q[(g_qhead + found) % QSIZE];
        if (flags & 1) {
            int k;
            for (k = found; k > 0; --k) {
                g_q[(g_qhead + k) % QSIZE] = g_q[(g_qhead + k - 1) % QSIZE];
            }
            g_qhead = (g_qhead + 1) % QSIZE;
            g_qcount--;
        }
    } else {
        /* WM_PAINT for windows with an update region */
        uint16_t ph = 0;
        if (!mn && !mx ? 1 : (mn <= 0x0F && 0x0F <= mx)) {
            for (i = 1; i < MAX_WND; ++i) {
                if (g_wnd[i].used && g_wnd[i].visible && g_wnd[i].has_update && (!hwnd || hwnd == i)) {
                    ph = (uint16_t)i;
                    break;
                }
            }
        }
        if (ph) {
            memset(&m, 0, sizeof(m));
            m.hwnd = ph;
            m.msg = 0x0F;
            m.time = now_ms();
        } else if (g_quit_posted) {
            memset(&m, 0, sizeof(m));
            m.msg = 0x12;
            m.wp = g_quit_code;
            if (flags & 1) g_quit_posted = 0;
        } else {
            /* Nothing to do: yield like Win16's PeekMessage did to other tasks. */
            SDL_Delay(1);
            AX = 0;
            return;
        }
    }
    if (lpmsg) {
        uint8_t *p = (uint8_t *)farp(lpmsg);
        memcpy(p, &m.hwnd, 2);
        memcpy(p + 2, &m.msg, 2);
        memcpy(p + 4, &m.wp, 2);
        memcpy(p + 6, &m.lp, 4);
        memcpy(p + 10, &m.time, 4);
        memcpy(p + 14, &m.x, 2);
        memcpy(p + 16, &m.y, 2);
    }
    AX = 1;
}

void api_USER_TRANSLATEMESSAGE(void)
{
    ArgReader a = args_pascal(4);
    uint8_t *p = (uint8_t *)farp(arg_d(&a));
    uint16_t hwnd, msg, vk;
    uint32_t lp;
    api_ret(4);
    AX = 0;
    if (!p) return;
    memcpy(&hwnd, p, 2);
    memcpy(&msg, p + 2, 2);
    memcpy(&vk, p + 4, 2);
    memcpy(&lp, p + 6, 4);
    if (msg == 0x100) {
        int shift = (g_keys[0x10] & 0x8000) != 0;
        int caps = g_keys[0x14] & 1;
        int ch = 0;
        if (vk >= 'A' && vk <= 'Z') ch = (shift ^ caps) ? vk : vk + 32;
        else if (vk >= '0' && vk <= '9') ch = shift ? ")!@#$%^&*("[vk - '0'] : vk;
        else if (vk == 0x20 || vk == 0x0D || vk == 0x08 || vk == 0x1B || vk == 0x09) ch = vk;
        else if (vk >= 0x60 && vk <= 0x69) ch = '0' + (vk - 0x60);
        else {
            switch (vk) {
                case 0xBD: ch = shift ? '_' : '-'; break;
                case 0xBB: ch = shift ? '+' : '='; break;
                case 0xBC: ch = shift ? '<' : ','; break;
                case 0xBE: ch = shift ? '>' : '.'; break;
                case 0xBF: ch = shift ? '?' : '/'; break;
                case 0xBA: ch = shift ? ':' : ';'; break;
                case 0xDE: ch = shift ? '"' : '\''; break;
                default: break;
            }
        }
        if (ch && !(g_keys[0x11] & 0x8000)) {
            post(hwnd, 0x102, (uint16_t)ch, lp);
            AX = 1;
        }
    }
}

void api_USER_DISPATCHMESSAGE(void)
{
    ArgReader a = args_pascal(4);
    uint8_t *p = (uint8_t *)farp(arg_d(&a));
    uint16_t hwnd, msg, wp;
    uint32_t lp, r;
    api_ret(4);
    memcpy(&hwnd, p, 2);
    memcpy(&msg, p + 2, 2);
    memcpy(&wp, p + 4, 2);
    memcpy(&lp, p + 6, 4);
    if (msg >= 0x100 && msg <= 0x105) api_log("Dispatch key msg %04x vk %02x", msg, wp);
    r = send_msg(hwnd, msg, wp, lp);
    if (msg == 0x0F) {
        Wnd *w = wnd_get(hwnd);
        if (w) w->has_update = 0; /* avoid endless WM_PAINT if not validated */
    }
    ret_d(r);
}

void api_USER_POSTMESSAGE(void)
{
    ArgReader a = args_pascal(10);
    uint16_t hwnd = arg_w(&a), msg = arg_w(&a), wp = arg_w(&a);
    uint32_t lp = arg_d(&a);
    post(hwnd, msg, wp, lp);
    AX = 1;
    api_ret(10);
}

void api_USER_POSTQUITMESSAGE(void)
{
    ArgReader a = args_pascal(2);
    g_quit_code = arg_w(&a);
    g_quit_posted = 1;
    api_ret(2);
}

void api_USER_GETKEYSTATE(void)
{
    ArgReader a = args_pascal(2);
    int16_t vk = arg_i(&a);
    AX = (vk >= 0 && vk < 256) ? (uint16_t)g_keys[vk] : 0;
    if (AX & 0x8000) api_log("GetKeyState(%02x) = down", vk);
    api_ret(2);
}

void api_USER_GETASYNCKEYSTATE(void)
{
    ArgReader a = args_pascal(2);
    int16_t vk = arg_i(&a);
    user_idle();
    AX = (vk >= 0 && vk < 256) ? (uint16_t)(g_keys[vk] & 0x8000) : 0;
    api_log("GetAsyncKeyState(%02x) = %04x", vk, AX);
    api_ret(2);
}

void api_KEYBOARD_VKKEYSCAN(void)
{
    ArgReader a = args_pascal(2);
    uint8_t ch = (uint8_t)arg_w(&a);
    if (ch >= 'a' && ch <= 'z') AX = (uint16_t)(ch - 32);
    else if (ch >= 'A' && ch <= 'Z') AX = (uint16_t)(0x100 | ch);
    else if (ch >= '0' && ch <= '9') AX = ch;
    else if (ch == ' ') AX = 0x20;
    else AX = 0xFFFF;
    api_ret(2);
}

void api_USER_LOADCURSOR(void)
{
    ArgReader a = args_pascal(6);
    uint16_t hinst = arg_w(&a);
    uint32_t name = arg_d(&a);
    const uint8_t *d;
    uint32_t len;
    AX = 0;
    if (!hinst) {
        AX = cursor_builtin((uint16_t)name == 32514 ? 1 : 0);
    } else if ((name >> 16) == 0) {
        /* RT_GROUP_CURSOR -> RT_CURSOR */
        if (kernel_find_resource_raw(0, 12, (uint16_t)name, &d, &len) == 0 && len >= 6 + 14) {
            uint16_t id = (uint16_t)(d[6 + 12] | (d[6 + 13] << 8));
            if (kernel_find_resource_raw(0, 1, id, &d, &len) == 0) {
                AX = cursor_from_resource(d, len);
            }
        }
    } else {
        char n[64];
        far_to_c(name, n, sizeof(n));
        fprintf(stderr, "[recomp] LoadCursor by name '%s' not supported\n", n);
        AX = cursor_builtin(0);
    }
    api_log("LoadCursor(%04x, %08x) = %04x", hinst, name, AX);
    api_ret(6);
}

void api_USER_SETCURSOR(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    AX = g_cursor;
    g_cursor = h;
    api_ret(2);
}

void api_USER_GETCURSOR(void) { AX = g_cursor; api_ret(0); }

void api_USER_SHOWCURSOR(void)
{
    ArgReader a = args_pascal(2);
    int16_t s = arg_i(&a);
    g_cursor_count += s ? 1 : -1;
    AX = (uint16_t)g_cursor_count;
    api_ret(2);
}

void api_USER_DESTROYCURSOR(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    if (h >= CUR_BASE && h < CUR_BASE + MAX_CURSOR && h != g_cursor) {
        g_cur[h - CUR_BASE].used = 0;
    }
    AX = 1;
    api_ret(2);
}

void api_USER_MESSAGEBOX(void)
{
    ArgReader a = args_pascal(12);
    char text[512], cap[128];
    uint16_t type;
    (void)arg_w(&a);
    far_to_c(arg_d(&a), text, sizeof(text));
    far_to_c(arg_d(&a), cap, sizeof(cap));
    type = arg_w(&a);
    fprintf(stderr, "[MessageBox] %s: %s (type %04x)\n", cap, text, type);
    switch (type & 0xF) {
        case 1: AX = 1; break;  /* OKCANCEL -> OK */
        case 3: case 4: AX = 6; break; /* YESNO(CANCEL) -> YES */
        case 5: AX = 4; break;  /* RETRYCANCEL -> RETRY */
        default: AX = 1; break;
    }
    api_ret(12);
}

/* ------------------------------------------------------------ rects */

void api_USER_SETRECT(void)
{
    ArgReader a = args_pascal(12);
    uint32_t rc = arg_d(&a);
    int16_t l = arg_i(&a), t = arg_i(&a), r = arg_i(&a), b = arg_i(&a);
    put_rect(rc, l, t, r, b);
    api_ret(12);
}

void api_USER_ISRECTEMPTY(void)
{
    ArgReader a = args_pascal(4);
    int16_t *r = (int16_t *)farp(arg_d(&a));
    AX = (r[2] <= r[0] || r[3] <= r[1]) ? 1 : 0;
    api_ret(4);
}

void api_USER_EQUALRECT(void)
{
    ArgReader a = args_pascal(8);
    int16_t *x = (int16_t *)farp(arg_d(&a)), *y = (int16_t *)farp(arg_d(&a));
    AX = memcmp(x, y, 8) == 0;
    api_ret(8);
}

void api_USER_OFFSETRECT(void)
{
    ArgReader a = args_pascal(8);
    int16_t *r = (int16_t *)farp(arg_d(&a));
    int16_t dx = arg_i(&a), dy = arg_i(&a);
    r[0] = (int16_t)(r[0] + dx); r[2] = (int16_t)(r[2] + dx);
    r[1] = (int16_t)(r[1] + dy); r[3] = (int16_t)(r[3] + dy);
    AX = 1;
    api_ret(8);
}

void api_USER_INTERSECTRECT(void)
{
    ArgReader a = args_pascal(12);
    int16_t *d = (int16_t *)farp(arg_d(&a));
    int16_t x[4], y[4], r[4];
    memcpy(x, farp(arg_d(&a)), 8);
    memcpy(y, farp(arg_d(&a)), 8);
    r[0] = x[0] > y[0] ? x[0] : y[0];
    r[1] = x[1] > y[1] ? x[1] : y[1];
    r[2] = x[2] < y[2] ? x[2] : y[2];
    r[3] = x[3] < y[3] ? x[3] : y[3];
    if (r[2] <= r[0] || r[3] <= r[1]) {
        memset(r, 0, 8);
        AX = 0;
    } else {
        AX = 1;
    }
    memcpy(d, r, 8);
    api_ret(12);
}

void api_USER_UNIONRECT(void)
{
    ArgReader a = args_pascal(12);
    int16_t *d = (int16_t *)farp(arg_d(&a));
    int16_t x[4], y[4], r[4];
    int xe, ye;
    memcpy(x, farp(arg_d(&a)), 8);
    memcpy(y, farp(arg_d(&a)), 8);
    xe = x[2] <= x[0] || x[3] <= x[1];
    ye = y[2] <= y[0] || y[3] <= y[1];
    if (xe && ye) {
        memset(r, 0, 8);
        AX = 0;
    } else if (xe) {
        memcpy(r, y, 8);
        AX = 1;
    } else if (ye) {
        memcpy(r, x, 8);
        AX = 1;
    } else {
        r[0] = x[0] < y[0] ? x[0] : y[0];
        r[1] = x[1] < y[1] ? x[1] : y[1];
        r[2] = x[2] > y[2] ? x[2] : y[2];
        r[3] = x[3] > y[3] ? x[3] : y[3];
        AX = 1;
    }
    memcpy(d, r, 8);
    api_ret(12);
}

void api_USER_PTINRECT(void)
{
    ArgReader a = args_pascal(8);
    int16_t *r = (int16_t *)farp(arg_d(&a));
    uint32_t pt = arg_d(&a);
    int16_t x = (int16_t)pt, y = (int16_t)(pt >> 16);
    AX = (x >= r[0] && x < r[2] && y >= r[1] && y < r[3]) ? 1 : 0;
    api_ret(8);
}

/* ------------------------------------------------------------ wsprintf */

void api_USER__WSPRINTF(void)
{
    ArgReader a = args_cdecl();
    uint32_t out = carg_d(&a), fmtp = carg_d(&a);
    char fmt[512], res[1024];
    size_t o = 0;
    const char *f;
    far_to_c(fmtp, fmt, sizeof(fmt));
    for (f = fmt; *f && o < sizeof(res) - 1; ++f) {
        char spec[32];
        size_t sl = 0;
        int is_long = 0;
        char tmp[512];
        if (*f != '%') {
            res[o++] = *f;
            continue;
        }
        spec[sl++] = '%';
        ++f;
        while (*f && strchr("-#0 +", *f) && sl < 20) spec[sl++] = *f++;
        while (*f && isdigit((unsigned char)*f) && sl < 20) spec[sl++] = *f++;
        if (*f == '.') { spec[sl++] = *f++; while (*f && isdigit((unsigned char)*f) && sl < 24) spec[sl++] = *f++; }
        if (*f == 'l' || *f == 'L') { is_long = 1; ++f; }
        if (!*f) break;
        switch (*f) {
            case 'd': case 'i':
                spec[sl++] = 'l'; spec[sl++] = 'd'; spec[sl] = 0;
                snprintf(tmp, sizeof(tmp), spec, is_long ? (long)(int32_t)carg_d(&a) : (long)(int16_t)carg_w(&a));
                break;
            case 'u': case 'x': case 'X':
                spec[sl++] = 'l'; spec[sl++] = *f; spec[sl] = 0;
                snprintf(tmp, sizeof(tmp), spec, is_long ? (unsigned long)carg_d(&a) : (unsigned long)carg_w(&a));
                break;
            case 'c':
                spec[sl++] = 'c'; spec[sl] = 0;
                snprintf(tmp, sizeof(tmp), spec, (int)(uint8_t)carg_w(&a));
                break;
            case 's': {
                char s[512];
                far_to_c(carg_d(&a), s, sizeof(s));
                spec[sl++] = 's'; spec[sl] = 0;
                snprintf(tmp, sizeof(tmp), spec, s);
                break;
            }
            case '%':
                snprintf(tmp, sizeof(tmp), "%%");
                break;
            default:
                tmp[0] = *f; tmp[1] = 0;
                break;
        }
        {
            size_t n = strlen(tmp);
            if (o + n >= sizeof(res)) n = sizeof(res) - 1 - o;
            memcpy(res + o, tmp, n);
            o += n;
        }
    }
    res[o] = 0;
    c_to_far(res, out, o + 1);
    AX = (uint16_t)o;
    api_ret_cdecl();
}

/* SHELL.5 RegSetValue: registration database writes are irrelevant here. */
void api_SHELL_SHELL_5(void)
{
    ret_d(0);
    api_ret(20);
}

#include "win16_user.h"
#include "win16_waveout.h"

#include <ctype.h>
#include <string.h>

#define SB_MAX_WND_CLASSES 32
#define SB_MAX_WINDOWS     64
#define SB_MAX_MSG_QUEUE   256

typedef struct SBWndSlot {
    uint8_t in_use;
    uint8_t visible;
    uint8_t has_dirty_rect;
    SB_HWND handle;
    SB_HWND parent;
    char class_name[64];
    char title[64];
    uint32_t style;
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    SB_HDC hdc;
    void *user_ptr;
    SB_RECT16 dirty_rect;
} SBWndSlot;

static char g_registered_classes[SB_MAX_WND_CLASSES][64];
static uint16_t g_registered_class_count = 0;
static SBWndSlot g_windows[SB_MAX_WINDOWS];
static SB_MSG16 g_msg_queue[SB_MAX_MSG_QUEUE];
static uint16_t g_msg_count = 0;
static int16_t g_key_states[256];
static SB_HCURSOR g_active_cursor = 1;
static SB_HDC g_screen_dc = 0;
static SB_HBITMAP g_screen_bmp = 0;
static int16_t g_window_org_x = 0;
static int16_t g_window_org_y = 0;

static void ensure_screen_dc(void)
{
    if (g_screen_dc == 0) {
        g_screen_dc = sb_win16_CreateCompatibleDC(0);
        g_screen_bmp = sb_win16_CreateBitmap(640, 480, 1, 8, NULL);
        sb_win16_SelectObject(g_screen_dc, g_screen_bmp);
    }
}

void sb_win16_user_shim_reset(void)
{
    memset(g_registered_classes, 0, sizeof(g_registered_classes));
    g_registered_class_count = 0;
    memset(g_windows, 0, sizeof(g_windows));
    memset(g_msg_queue, 0, sizeof(g_msg_queue));
    g_msg_count = 0;
    memset(g_key_states, 0, sizeof(g_key_states));
    g_active_cursor = 1;
    g_screen_dc = 0;
    g_screen_bmp = 0;
    g_window_org_x = 0;
    g_window_org_y = 0;
}

static SBWndSlot *get_wnd_slot(SB_HWND hWnd)
{
    if (hWnd > 0 && hWnd < SB_MAX_WINDOWS && g_windows[hWnd].in_use) {
        return &g_windows[hWnd];
    }
    return NULL;
}

int sb_win16_GetClassInfo(uint16_t hInstance, const char *lpClassName, void *lpWndClass)
{
    uint16_t i;
    (void)hInstance;
    (void)lpWndClass;
    if (!lpClassName) {
        return 0;
    }
    for (i = 0; i < g_registered_class_count; ++i) {
        if (strcmp(g_registered_classes[i], lpClassName) == 0) {
            return 1;
        }
    }
    return 0;
}

uint16_t sb_win16_RegisterClass(const char *class_name)
{
    if (!class_name) {
        return 0;
    }
    if (sb_win16_GetClassInfo(0, class_name, NULL)) {
        return 1;
    }
    if (g_registered_class_count < SB_MAX_WND_CLASSES) {
        strncpy(g_registered_classes[g_registered_class_count], class_name, 63);
        g_registered_classes[g_registered_class_count][63] = '\0';
        g_registered_class_count++;
        return 1;
    }
    return 0;
}

SB_HWND sb_win16_CreateWindow(const char *lpClassName,
                              const char *lpWindowName,
                              uint32_t dwStyle,
                              int16_t x,
                              int16_t y,
                              int16_t nWidth,
                              int16_t nHeight,
                              SB_HWND hWndParent,
                              uint16_t hMenu,
                              uint16_t hInstance,
                              void *lpParam)
{
    uint16_t i;
    (void)hMenu;
    (void)hInstance;
    ensure_screen_dc();

    for (i = 1; i < SB_MAX_WINDOWS; ++i) {
        if (!g_windows[i].in_use) {
            SBWndSlot *w = &g_windows[i];
            memset(w, 0, sizeof(*w));
            w->in_use = 1;
            w->visible = 1;
            w->handle = i;
            w->parent = hWndParent;
            if (lpClassName) {
                strncpy(w->class_name, lpClassName, sizeof(w->class_name) - 1);
            }
            if (lpWindowName) {
                strncpy(w->title, lpWindowName, sizeof(w->title) - 1);
            }
            w->style = dwStyle;
            w->x = (x == (int16_t)0x8000) ? 0 : x;
            w->y = (y == (int16_t)0x8000) ? 0 : y;
            w->width = (nWidth <= 0 || nWidth == (int16_t)0x8000) ? 640 : nWidth;
            w->height = (nHeight <= 0 || nHeight == (int16_t)0x8000) ? 480 : nHeight;
            w->hdc = g_screen_dc;
            w->user_ptr = lpParam;
            return (SB_HWND)i;
        }
    }
    return 0;
}

int sb_win16_DestroyWindow(SB_HWND hWnd)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    uint16_t i;
    if (!w) {
        return 0;
    }
    /* Destroy child windows first */
    for (i = 1; i < SB_MAX_WINDOWS; ++i) {
        if (g_windows[i].in_use && g_windows[i].parent == hWnd) {
            sb_win16_DestroyWindow(i);
        }
    }
    memset(w, 0, sizeof(*w));
    return 1;
}

int sb_win16_ShowWindow(SB_HWND hWnd, int nCmdShow)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    if (!w) {
        return 0;
    }
    w->visible = (nCmdShow != 0) ? 1 : 0;
    return 1;
}

SB_HWND sb_win16_GetWindow(SB_HWND hWnd, uint16_t uCmd)
{
    uint16_t i;
    if (uCmd == SB_GW_CHILD) {
        for (i = 1; i < SB_MAX_WINDOWS; ++i) {
            if (g_windows[i].in_use && g_windows[i].parent == hWnd) {
                return i;
            }
        }
        return 0;
    }
    if (uCmd == SB_GW_HWNDNEXT) {
        SBWndSlot *cur = get_wnd_slot(hWnd);
        if (!cur) {
            return 0;
        }
        for (i = (uint16_t)(hWnd + 1u); i < SB_MAX_WINDOWS; ++i) {
            if (g_windows[i].in_use && g_windows[i].parent == cur->parent) {
                return i;
            }
        }
        return 0;
    }
    return 0;
}

void *sb_win16_GetWindowLongPtr(SB_HWND hWnd, int nIndex)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    (void)nIndex;
    return w ? w->user_ptr : NULL;
}

void *sb_win16_SetWindowLongPtr(SB_HWND hWnd, int nIndex, void *dwNewLong)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    void *prev = NULL;
    (void)nIndex;
    if (w) {
        prev = w->user_ptr;
        w->user_ptr = dwNewLong;
    }
    return prev;
}

SB_HDC sb_win16_GetDC(SB_HWND hWnd)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    ensure_screen_dc();
    return w ? w->hdc : g_screen_dc;
}

int sb_win16_ReleaseDC(SB_HWND hWnd, SB_HDC hDC)
{
    (void)hWnd;
    (void)hDC;
    return 1;
}

uint32_t sb_win16_SetWindowOrg(SB_HDC hdc, int16_t x, int16_t y)
{
    uint32_t prev = ((uint32_t)(uint16_t)g_window_org_y << 16) | (uint16_t)g_window_org_x;
    (void)hdc;
    g_window_org_x = x;
    g_window_org_y = y;
    return prev;
}

uint32_t sb_win16_GetWindowOrg(SB_HDC hdc)
{
    (void)hdc;
    return ((uint32_t)(uint16_t)g_window_org_y << 16) | (uint16_t)g_window_org_x;
}

int sb_win16_GetDeviceCaps(SB_HDC hdc, int nIndex)
{
    (void)hdc;
    if (nIndex == 8) { /* HORZRES */
        return 640;
    }
    if (nIndex == 10) { /* VERTRES */
        return 480;
    }
    if (nIndex == 0x68) { /* SIZEPALETTE */
        return 256;
    }
    return 1;
}

void sb_win16_ClientToScreen(SB_HWND hWnd, SB_POINT16 *lpPoint)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    if (w && lpPoint) {
        lpPoint->x = (int16_t)(lpPoint->x + w->x);
        lpPoint->y = (int16_t)(lpPoint->y + w->y);
    }
}

void sb_win16_ScreenToClient(SB_HWND hWnd, SB_POINT16 *lpPoint)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    if (w && lpPoint) {
        lpPoint->x = (int16_t)(lpPoint->x - w->x);
        lpPoint->y = (int16_t)(lpPoint->y - w->y);
    }
}

int sb_win16_PtInRect(const SB_RECT16 *lprc, SB_POINT16 pt)
{
    if (!lprc) {
        return 0;
    }
    return (pt.x >= lprc->left && pt.x < lprc->right &&
            pt.y >= lprc->top && pt.y < lprc->bottom)
               ? 1
               : 0;
}

void sb_win16_InvalidateRect(SB_HWND hWnd, const SB_RECT16 *lpRect, int bErase)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    (void)bErase;
    if (w) {
        if (lpRect) {
            if (w->has_dirty_rect) {
                sb_win16_UnionRect(&w->dirty_rect, &w->dirty_rect, lpRect);
            } else {
                w->dirty_rect = *lpRect;
                w->has_dirty_rect = 1;
            }
        } else {
            sb_win16_SetRect(&w->dirty_rect, 0, 0, w->width, w->height);
            w->has_dirty_rect = 1;
        }
    }
}

void sb_win16_ValidateRect(SB_HWND hWnd, const SB_RECT16 *lpRect)
{
    SBWndSlot *w = get_wnd_slot(hWnd);
    (void)lpRect;
    if (w) {
        w->has_dirty_rect = 0;
    }
}

SB_HCURSOR sb_win16_LoadCursor(uint16_t hInstance, uint16_t lpCursorName)
{
    (void)hInstance;
    return (SB_HCURSOR)(lpCursorName ? lpCursorName : 1u);
}

SB_HCURSOR sb_win16_SetCursor(SB_HCURSOR hCursor)
{
    SB_HCURSOR prev = g_active_cursor;
    g_active_cursor = hCursor;
    return prev;
}

int sb_win16_DestroyCursor(SB_HCURSOR hCursor)
{
    (void)hCursor;
    return 1;
}

SB_HGDIOBJ sb_win16_GetStockObject(int fnObject)
{
    return (SB_HGDIOBJ)(0x4000u | (fnObject & 0xFFu));
}

int sb_win16_PostMessage(SB_HWND hWnd, uint16_t Msg, uint16_t wParam, uint32_t lParam)
{
    SB_MSG16 *m;
    if (g_msg_count >= SB_MAX_MSG_QUEUE) {
        return 0;
    }
    m = &g_msg_queue[g_msg_count++];
    m->hwnd = hWnd;
    m->message = Msg;
    m->wParam = wParam;
    m->lParam = lParam;
    m->time = sb_win16_timeGetTime();
    m->pt.x = (int16_t)(lParam & 0xFFFFu);
    m->pt.y = (int16_t)((lParam >> 16) & 0xFFFFu);
    return 1;
}

int sb_win16_PeekMessage(SB_MSG16 *lpMsg,
                         SB_HWND hWnd,
                         uint16_t wMsgFilterMin,
                         uint16_t wMsgFilterMax,
                         uint16_t wRemoveMsg)
{
    uint16_t i;
    for (i = 0; i < g_msg_count; ++i) {
        const SB_MSG16 *m = &g_msg_queue[i];
        int match_wnd = (hWnd == 0 || m->hwnd == hWnd);
        int match_msg = ((wMsgFilterMin == 0 && wMsgFilterMax == 0) ||
                         (m->message >= wMsgFilterMin && m->message <= wMsgFilterMax));
        if (match_wnd && match_msg) {
            if (lpMsg) {
                *lpMsg = *m;
            }
            if ((wRemoveMsg & SB_PM_REMOVE) != 0) {
                if (i + 1 < g_msg_count) {
                    memmove(&g_msg_queue[i],
                            &g_msg_queue[i + 1],
                            (size_t)(g_msg_count - i - 1) * sizeof(SB_MSG16));
                }
                g_msg_count--;
            }
            return 1;
        }
    }
    /* Synthesize WM_PAINT if any visible window has an invalidated dirty rect and filter allows 0x0F */
    if (wMsgFilterMin == 0 || (wMsgFilterMin <= SB_WM_PAINT && SB_WM_PAINT <= wMsgFilterMax)) {
        for (i = 1; i < SB_MAX_WINDOWS; ++i) {
            if (g_windows[i].in_use && g_windows[i].visible && g_windows[i].has_dirty_rect &&
                (hWnd == 0 || hWnd == i)) {
                if (lpMsg) {
                    memset(lpMsg, 0, sizeof(*lpMsg));
                    lpMsg->hwnd = i;
                    lpMsg->message = SB_WM_PAINT;
                    lpMsg->time = sb_win16_timeGetTime();
                }
                if ((wRemoveMsg & SB_PM_REMOVE) != 0) {
                    g_windows[i].has_dirty_rect = 0;
                }
                return 1;
            }
        }
    }
    return 0;
}

int sb_win16_TranslateMessage(const SB_MSG16 *lpMsg)
{
    (void)lpMsg;
    return 1;
}

uint32_t sb_win16_DispatchMessage(const SB_MSG16 *lpMsg)
{
    (void)lpMsg;
    return 0;
}

int16_t sb_win16_GetKeyState(int nVirtKey)
{
    if (nVirtKey >= 0 && nVirtKey < 256) {
        return g_key_states[nVirtKey];
    }
    return 0;
}

void sb_win16_SetKeyState(int nVirtKey, int16_t state)
{
    if (nVirtKey >= 0 && nVirtKey < 256) {
        g_key_states[nVirtKey] = state;
    }
}

uint16_t sb_win16_VkKeyScan(char ch)
{
    unsigned char uc = (unsigned char)ch;
    if (uc >= 'a' && uc <= 'z') {
        return (uint16_t)toupper(uc);
    }
    if (uc >= 'A' && uc <= 'Z') {
        return (uint16_t)(uc | 0x0100u);
    }
    return (uint16_t)uc;
}

SB_HBITMAP sb_win16_user_getScreenBitmap(void)
{
    ensure_screen_dc();
    return g_screen_bmp;
}

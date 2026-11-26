/* Thin Win16 API Shim — USER Windowing, Cursors, Keyboard & Message Queue
 *
 * Implements the Win16 USER and KEYBOARD APIs imported by original/BLIZZARD.EXE
 * (`CWindow`, `CDesktop`, `CView`, `CEventDispatcher`, `CApplication`):
 *   - RegisterClass / GetClassInfo / CreateWindow / DestroyWindow / ShowWindow
 *   - GetDC / ReleaseDC / SetWindowOrg / GetWindowOrg / SelectClipRgn
 *   - InvalidateRect / ValidateRect / PtInRect / ClientToScreen / ScreenToClient
 *   - GetWindow / GetWindowLong / SetWindowLong
 *   - LoadCursor / SetCursor / DestroyCursor / GetStockObject
 *   - PeekMessage / TranslateMessage / DispatchMessage / PostMessage
 *   - GetKeyState / VkKeyScan (KEYBOARD.129)
 */
#ifndef SB_SHIM_WIN16_USER_H
#define SB_SHIM_WIN16_USER_H

#include <stddef.h>
#include <stdint.h>
#include "win16_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t SB_HWND;
typedef uint16_t SB_HCURSOR;

/* Standard Win16 Window Messages */
#define SB_WM_CREATE        0x0001u
#define SB_WM_DESTROY       0x0002u
#define SB_WM_MOVE          0x0003u
#define SB_WM_SIZE          0x0005u
#define SB_WM_ACTIVATE      0x0006u
#define SB_WM_SETFOCUS      0x0007u
#define SB_WM_KILLFOCUS     0x0008u
#define SB_WM_PAINT         0x000Fu
#define SB_WM_CLOSE         0x0010u
#define SB_WM_ACTIVATEAPP   0x001Cu
#define SB_WM_SETCURSOR     0x0020u
#define SB_WM_KEYDOWN       0x0100u
#define SB_WM_KEYUP         0x0101u
#define SB_WM_CHAR          0x0102u
#define SB_WM_SYSKEYDOWN    0x0104u
#define SB_WM_SYSKEYUP      0x0105u
#define SB_WM_COMMAND       0x0111u
#define SB_WM_SYSCOMMAND    0x0112u
#define SB_WM_MOUSEMOVE     0x0200u
#define SB_WM_LBUTTONDOWN   0x0201u
#define SB_WM_LBUTTONUP     0x0202u
#define SB_WM_LBUTTONDBLCLK 0x0203u
#define SB_WM_RBUTTONDOWN   0x0204u
#define SB_WM_RBUTTONUP     0x0205u
#define SB_WM_RBUTTONDBLCLK 0x0206u
#define SB_WM_MBUTTONDOWN   0x0207u
#define SB_WM_MBUTTONUP     0x0208u
#define SB_WM_MBUTTONDBLCLK 0x0209u
#define SB_WM_USER          0x0400u

/* Win16 PeekMessage flags */
#define SB_PM_NOREMOVE 0x0000u
#define SB_PM_REMOVE   0x0001u

/* Win16 GetWindow constants */
#define SB_GW_HWNDNEXT 2u
#define SB_GW_CHILD    5u

/* Win16 MSG structure (18 bytes / 0x12) */
typedef struct SB_MSG16 {
    SB_HWND hwnd;       /* +0x00 */
    uint16_t message;   /* +0x02 */
    uint16_t wParam;    /* +0x04 */
    uint32_t lParam;    /* +0x06 */
    uint32_t time;      /* +0x0a */
    SB_POINT16 pt;      /* +0x0e..+0x11 */
} SB_MSG16;

/* Reset/shutdown all USER windowing and message queue state */
void sb_win16_user_shim_reset(void);

/* Window Class & Window Lifecycle */
int sb_win16_GetClassInfo(uint16_t hInstance, const char *lpClassName, void *lpWndClass);
uint16_t sb_win16_RegisterClass(const char *class_name);
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
                              void *lpParam);
int sb_win16_DestroyWindow(SB_HWND hWnd);
int sb_win16_ShowWindow(SB_HWND hWnd, int nCmdShow);
SB_HWND sb_win16_GetWindow(SB_HWND hWnd, uint16_t uCmd);
void *sb_win16_GetWindowLongPtr(SB_HWND hWnd, int nIndex);
void *sb_win16_SetWindowLongPtr(SB_HWND hWnd, int nIndex, void *dwNewLong);

/* Window DC, Coordinates & Regions */
SB_HDC sb_win16_GetDC(SB_HWND hWnd);
int sb_win16_ReleaseDC(SB_HWND hWnd, SB_HDC hDC);
uint32_t sb_win16_SetWindowOrg(SB_HDC hdc, int16_t x, int16_t y);
uint32_t sb_win16_GetWindowOrg(SB_HDC hdc);
int sb_win16_GetDeviceCaps(SB_HDC hdc, int nIndex);
void sb_win16_ClientToScreen(SB_HWND hWnd, SB_POINT16 *lpPoint);
void sb_win16_ScreenToClient(SB_HWND hWnd, SB_POINT16 *lpPoint);
int sb_win16_PtInRect(const SB_RECT16 *lprc, SB_POINT16 pt);
void sb_win16_InvalidateRect(SB_HWND hWnd, const SB_RECT16 *lpRect, int bErase);
void sb_win16_ValidateRect(SB_HWND hWnd, const SB_RECT16 *lpRect);

/* Cursor & Stock Object APIs */
SB_HCURSOR sb_win16_LoadCursor(uint16_t hInstance, uint16_t lpCursorName);
SB_HCURSOR sb_win16_SetCursor(SB_HCURSOR hCursor);
int sb_win16_DestroyCursor(SB_HCURSOR hCursor);
SB_HGDIOBJ sb_win16_GetStockObject(int fnObject);

/* Message Queue & Keyboard APIs */
int sb_win16_PostMessage(SB_HWND hWnd, uint16_t Msg, uint16_t wParam, uint32_t lParam);
int sb_win16_PeekMessage(SB_MSG16 *lpMsg,
                         SB_HWND hWnd,
                         uint16_t wMsgFilterMin,
                         uint16_t wMsgFilterMax,
                         uint16_t wRemoveMsg);
int sb_win16_TranslateMessage(const SB_MSG16 *lpMsg);
uint32_t sb_win16_DispatchMessage(const SB_MSG16 *lpMsg);

int16_t sb_win16_GetKeyState(int nVirtKey);
void sb_win16_SetKeyState(int nVirtKey, int16_t state);
uint16_t sb_win16_VkKeyScan(char ch);

/* Screen framebuffer inspection for verification */
SB_HBITMAP sb_win16_user_getScreenBitmap(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_SHIM_WIN16_USER_H */

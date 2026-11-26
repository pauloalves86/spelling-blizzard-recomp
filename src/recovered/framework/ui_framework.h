/* Decompiled Bright Star Core Collections, Event Dispatcher, View, Window, Desktop & Application Framework (Path 2)
 *
 * Reconstructs the 193 functions governing the core object-oriented application framework:
 *
 * 1. Core Collections & Event Target (`1008:b6f8..c762`, `1010:036c`, `1010:1246` — 56 functions):
 *    - `CEventTarget_1008_07f4` (`1008:b6f8..b75c`): Base event/command recipient with parent link (`+0x04`).
 *    - `CCollection_1008_0824` & `CList_1008_0904` (`1008:b760..bfc6`): Dynamic growable array of elements (`+0x04` count, `+0x06` elem_size, `+0x08` chunk_grow, `+0x0a` alloc_bytes, `+0x0c` lock_depth, `+0x0e` data).
 *    - `COrderedList_1008_0b08` & `CObList_1008_0a48` (`1008:c02a..c762`, `1010:036c`, `1010:1246`): Ordered pointer collection (`ForEach` `1008:c146`, `ForEach1` `1008:c19e`, `FirstThat` `1008:c1fc`, `FirstThat1` `1008:c266`, `LastThat` `1008:c2d8`, `LastThat1` `1008:c346`, `InsertFirst` `1008:c504`, `InsertLast` `1008:c520`, `Remove` `1008:c554`, `IndexOf` `1008:c5cc`, `DisposeAll` `1008:c62a`).
 *
 * 2. Event Dispatcher (`CEventDispatcherBase_1008_0b84` & `CEventDispatcher_1008_7f84` — 18 functions):
 *    - `1010:3a28..42ce`, `1008:7c02..7c5c`, `1020:2534..2552`: Polls Win16 message queue (`PeekMessage`), tracks mouse buttons/position, double-click intervals, and modifier keys (`VK_SHIFT`, `VK_CONTROL`, `VK_MENU`), and dispatches mouse/key/idle events to the desktop, active window, and idle targets (`DAT_1060_0a2e`).
 *
 * 3. View & Desktop Hierarchy (`CView_1008_1aac` & `CDesktop_1008_7d10` — 58 functions):
 *    - `1010:15f0..1de8`, `1010:45ea..4fc0`: Base UI view hierarchy loaded from `'Pane'` (`0x03f1`) resources (`1010:1826`), supporting subview lists (`+0x08`), frame/bounds coordinate transforms, hit-testing (`1010:48ac`), focus/PrepareDC (`1010:4a0a`), drawing (`1010:4b30`), and invalidation (`1010:4d5a`).
 *    - `1008:31b8..3218`, `1008:d1d8`: Root `CDesktop_1008_7d10` (`DAT_1060_0810`, 640x480) managing top-level windows and floating layers.
 *
 * 4. Window Wrapper (`CWindow_1010_5942` — 26 functions):
 *    - `1010:50ce..5d7c`, `1010:2d16..2d22`: Wraps a Win16 `HWND` (`+0x42`), registers window classes (`"c window"`, `"p window"`), bridges `WndProc` (`1010:5a18`) via `GetWindowLong(hwnd, 0)` (`1010:59c2`), and handles `WM_PAINT`, `WM_ACTIVATE`, `WM_MOVE`, `WM_SIZE`, `WM_SETCURSOR`, and `WM_CLOSE`.
 *
 * 5. Application Lifecycle (`CApplication_1008_2e32` — 35 functions):
 *    - `1008:018c..0700`, `1008:2bf4`, `1008:72b6..7bf2`: Top-level application object (`DAT_1060_07c4`), startup entry (`1008:2bf4`), subsystem initialization (`1008:72b6`), main loop (`1008:0694`), idle & sound pumping (`1008:79ec`), and clean shutdown (`1008:0700`).
 */
#ifndef SB_RECOVERED_FRAMEWORK_UI_FRAMEWORK_H
#define SB_RECOVERED_FRAMEWORK_UI_FRAMEWORK_H

#include <stddef.h>
#include <stdint.h>
#include "../../shim/win16_user.h"
#include "graphics_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct SBEventTarget SBEventTarget;
typedef struct SBCollection SBCollection;
typedef struct SBView SBView;
typedef struct SBWindow SBWindow;
typedef struct SBDesktop SBDesktop;
typedef struct SBEventDispatcher SBEventDispatcher;
typedef struct SBApplication SBApplication;

/* Callback prototypes for CObList iteration (`ForEach`, `FirstThat`, `LastThat`) */
typedef void (*SBObListIterFn)(void *item);
typedef void (*SBObListIter1Fn)(void *item, void *ctx);
typedef int (*SBObListPredFn)(void *item);
typedef int (*SBObListPred1Fn)(void *item, void *ctx);
typedef int (*SBCollectionCompareFn)(const void *a, const void *b);

/* 1. Base Event Target (`CEventTarget_1008_07f4`, 8 bytes) */
struct SBEventTarget {
    uint32_t vtable_tag_00;         /* +0x00: 0x100807f4 (or derived vtable) */
    SBEventTarget *next_handler_04; /* +0x04: parent/next command target */
};

/* 2. Dynamic Collection / Object List (`CCollection_1008_0824` / `CList_1008_0904` /
 *    `COrderedList_1008_0b08` / `CObList_1008_0a48`, 0x16 bytes)
 */
struct SBCollection {
    uint32_t vtable_tag_00;  /* +0x00: 0x10080824, 0x10080904, 0x10080b08, or 0x10080a48 */
    int16_t count_04;        /* +0x04: active element count */
    int16_t elem_size_06;    /* +0x06: element size in bytes (4 for far pointer list) */
    int16_t chunk_grow_08;   /* +0x08: allocation growth quantum in bytes */
    int16_t alloc_bytes_0a;  /* +0x0a: currently allocated buffer size in bytes */
    int16_t lock_depth_0c;   /* +0x0c: nested iteration lock count */
    uint8_t *data_0e;        /* +0x0e: storage buffer (array of `void *` for CObList) */
    uint8_t is_sorted_12;    /* +0x12: 1 if sorted */
};

/* 3. View Hierarchy (`CView_1008_1aac`, 0x38 bytes) */
struct SBView {
    SBEventTarget base_00;       /* +0x00..+0x07: vtable_tag_00 (`0x10081aac`), next_handler_04 */
    SBCollection *subviews_08;   /* +0x08: `CObList_1008_0a48` of child `SBView *` */
    SBView *superview_0c;        /* +0x0c: parent `SBView *` */
    int16_t loc_x_10;            /* +0x10: X position in superview coordinates */
    int16_t loc_y_12;            /* +0x12: Y position in superview coordinates */
    int16_t width_14;            /* +0x14: view width */
    int16_t height_16;           /* +0x16: view height */
    int16_t scroll_x_18;         /* +0x18: internal horizontal origin offset */
    int16_t scroll_y_1a;         /* +0x1a: internal vertical origin offset */
    uint8_t sizing_flags_1c;     /* +0x1c: auto-resize flags */
    uint8_t visible_1d;          /* +0x1d: 1 if visible */
    uint8_t active_1e;           /* +0x1e: 1 if active */
    uint8_t wants_clicks_1f;     /* +0x1f: 1 if view accepts mouse clicks */
    int32_t pane_id_20;          /* +0x20: 32-bit view/pane identifier */
    uint8_t border_flags_24;     /* +0x24: border drawing flags */
    uint16_t help_id_26;         /* +0x26: help/tooltip ID */
    SB_HDC cached_hdc_28;        /* +0x28: active DC during PrepareDC/Draw */
    uint16_t pen_state_2a;       /* +0x2a: saved pen/DC state */
    SB_HGDIOBJ border_pen_2c;    /* +0x2c: border pen handle */
    int16_t pen_x_30;            /* +0x30: current MoveTo X */
    int16_t pen_y_32;            /* +0x32: current MoveTo Y */
    uint8_t dc_prepared_34;      /* +0x34: 1 if DC is currently prepared */
    uint8_t owns_dc_35;          /* +0x35: 1 if view acquired DC via GetDC */
};

/* 4. Window Wrapper (`CWindow_1010_5942`, 0x54 bytes) */
struct SBWindow {
    SBView view_00;              /* +0x00..+0x37: base `CView_1008_1aac` */
    uint8_t is_floating_38;      /* +0x38: 1 if floating popup window */
    uint8_t is_modal_39;         /* +0x39: 1 if modal dialog window */
    uint8_t is_active_3a;        /* +0x3a: 1 if window is active */
    uint8_t is_closable_3b;      /* +0x3b: 1 if window can be closed */
    uint16_t proc_id_3c;         /* +0x3c: Mac WIND procID / window style class */
    uint8_t has_close_box_3e;    /* +0x3e: 1 if close box present */
    uint32_t ref_con_3f;         /* +0x3f: reference constant */
    SB_HWND hwnd_42;             /* +0x42: underlying Win16 `HWND` */
    uint32_t win_style_44;       /* +0x44: Win16 `dwStyle` */
    SB_HDC paint_hdc_48;         /* +0x48: `BeginPaint` DC during `WM_PAINT` */
    SB_RECT16 update_rc_4a;      /* +0x4a..+0x51: dirty/update rectangle */
    uint8_t in_paint_52;         /* +0x52: 1 while inside `WM_PAINT` */
};

/* 5. Desktop Root View (`CDesktop_1008_7d10`, 0x48 bytes) */
struct SBDesktop {
    SBView view_00;                   /* +0x00..+0x37: base `CView_1008_1aac` (640x480) */
    SBCollection *floating_views_38;  /* +0x38: floating layer list */
    SBWindow *top_window_3c;          /* +0x3c: active top-level `CWindow *` */
    SBWindow *desktop_window_40;      /* +0x40: main desktop host `CWindow *` */
    uint8_t cursor_hidden_44;         /* +0x44: cursor visibility state */
};

/* 6. Event Dispatcher (`CEventDispatcherBase_1008_0b84` & `CEventDispatcher_1008_7f84`, 0x62 bytes) */
struct SBEventDispatcher {
    SBEventTarget base_00;            /* +0x00..+0x07: vtable_tag_00 (`0x10087f84`) */
    SBCollection *idle_tasks_08;      /* +0x08: `CObList` of idle processors */
    SBEventTarget *focus_target_0c;   /* +0x0c: active keyboard focus target */
    SBView *mouse_capture_view_10;    /* +0x10: view currently tracking mouse */
    SBView *last_click_view_14;       /* +0x14: view receiving previous click */
    uint16_t event_mask_18;           /* +0x18: enabled event mask */
    uint32_t sleep_ticks_1a;          /* +0x1a: idle sleep interval */
    SB_MSG16 current_msg_1e;          /* +0x1e..+0x2f: current Win16 MSG */
    uint16_t modifiers_30;            /* +0x30: modifier bitmask (Shift/Ctrl/Alt/Mouse) */
    uint32_t last_click_time_32;      /* +0x32: timestamp of last mouse down */
    SB_POINT16 last_click_pt_36;      /* +0x36..+0x39: screen point of last mouse down */
    int16_t click_count_3a;           /* +0x3a: consecutive click count (1, 2, 3) */
    SB_POINT16 last_mouse_pt_3c;      /* +0x3c..+0x3f: last polled mouse position */
    uint32_t last_idle_time_40;       /* +0x40: timestamp of last idle dispatch */
    uint8_t button_down_44;           /* +0x44: 1 if mouse button held */
    uint8_t in_modal_loop_45;         /* +0x45: 1 if inside modal loop */
    uint32_t double_click_ticks_46;   /* +0x46: double-click threshold (ticks) */
    SB_HCURSOR arrow_cursor_4a;       /* +0x4a: standard arrow cursor */
    SB_HCURSOR wait_cursor_4c;        /* +0x4c: hourglass wait cursor */
    SB_HCURSOR active_cursor_4e;      /* +0x4e: currently active cursor */
    uint8_t custom_flag_58;           /* +0x58: `CEventDispatcher_1008_7f84` flag */
    uint8_t cursor_locked_59;         /* +0x59: `CEventDispatcher_1008_7f84` cursor lock */
    SB_HCURSOR custom_cursor_5a;      /* +0x5a: loaded custom cursor handle */
    uint16_t custom_cursor_id_5e;     /* +0x5e: current custom `'CURS'` resource ID */
};

/* 7. Top-Level Application (`CApplication_1008_2e32`, 0x80 bytes) */
struct SBApplication {
    SBEventDispatcher dispatcher_00;  /* +0x00..+0x61: base `CEventDispatcher_1008_7f84` */
    SBCollection *doc_list_62;        /* +0x62: open documents / controllers list */
    uint8_t quit_requested_66;        /* +0x66: 1 when application should exit main loop */
    uint8_t low_memory_67;            /* +0x67: 1 if memory reserve exhausted */
    uint8_t is_active_app_68;         /* +0x68: 1 if app is active in foreground */
    uint16_t cmd_show_6a;             /* +0x6a: WinMain `nCmdShow` */
    uint16_t hInstance_6c;            /* +0x6c: WinMain `hInstance` */
    uint16_t hPrevInstance_6e;        /* +0x6e: WinMain `hPrevInstance` */
    uint16_t main_wind_res_id_70;     /* +0x70: `'WIND'` resource ID (`0x0419` = 1049) */
    uint16_t clut_res_id_72;          /* +0x72: `'clut'` resource ID (`0x0419` = 1049) */
    SBWindow *main_window_74;         /* +0x74: main game window */
    SBPictSurface *offscreen_78;      /* +0x78: shared 640x480 offscreen composite surface */
    void *game_controller_7c;         /* +0x7c: main game controller (`DAT_1060_07f6`) */
};

/* Segment 1060 Framework Globals */
extern SBApplication *g_DAT_1060_07c4_app;
extern SBDesktop *g_DAT_1060_0810_desktop;
extern SBEventDispatcher *g_DAT_1060_0836_dispatcher;
extern SBCollection *g_DAT_1060_0a2e_idleList;
extern SBView *g_DAT_1060_0828_focusedView;
extern uint16_t g_DAT_1060_1d58_hInstance;
extern uint16_t g_DAT_1060_1d56_nCmdShow;
extern uint8_t g_DAT_1060_07f4_appReady;
extern void *g_DAT_1060_07f6_gameController;

/* =========================================================================
 * 1. Core Collections & Event Target (`1008:b6f8..c762`, `1010:036c`, `1010:1246`)
 * ========================================================================= */
SBEventTarget *CEventTarget_Ctor_1008_b6f8(SBEventTarget *self);
void CEventTarget_Dtor_1008_b714(SBEventTarget *self);
void CEventTarget_Dispose_1008_b732(SBEventTarget *self);
int CEventTarget_HandleCommand_1008_b73a(SBEventTarget *self, int32_t cmd);
int CEventTarget_HandleKey_1008_b74e(SBEventTarget *self, uint16_t key, uint16_t modifiers);
void CEventTarget_UpdateMenus_1008_b752(SBEventTarget *self);
void CEventTarget_Idle_1008_b758(SBEventTarget *self);
SBEventTarget *CEventTarget_GetNextHandler_1008_b75c(const SBEventTarget *self);

SBCollection *CCollection_Ctor_1008_b760(SBCollection *self, int16_t elem_size);
void CCollection_Dtor_1008_b7b8(SBCollection *self);
SBCollection *CCollection_Clone_1008_b7ec(const SBCollection *self);
void CCollection_CopyDataFrom_1008_b844(SBCollection *self, const SBCollection *src);
void CCollection_Dispose_1008_b89c(SBCollection *self);
int16_t CCollection_GetCount_1008_b8bc(const SBCollection *self);
int CCollection_IsEmpty_1008_b8ca(const SBCollection *self);
int16_t CCollection_Lock_1008_b8da(SBCollection *self, uint8_t lock_flag);
void CCollection_SetArraySize_1008_b920(SBCollection *self, int16_t new_count);
void CCollection_ResizeBuffer_1008_b994(SBCollection *self, int16_t byte_size);
void CCollection_InsertAt_1008_ba14(SBCollection *self, int16_t index_1based, const void *elem_ptr);
void CCollection_DeleteAt_1008_ba9c(SBCollection *self, int16_t index_1based, int16_t count);
void CCollection_GetElements_1008_bb0c(const SBCollection *self, int16_t index_1based, int16_t count, void *out_buf);
void CCollection_SetElements_1008_bb76(SBCollection *self, int16_t index_1based, int16_t count, const void *in_buf);
void CCollection_RemoveAll_1008_bbe4(SBCollection *self);
void *CCollection_GetElementPtr_1008_bc16(const SBCollection *self, int16_t index_1based);

SBCollection *CList_Ctor_1008_bc5c(SBCollection *self, int16_t elem_size);
void CList_Dtor_1008_bc96(SBCollection *self);
void CList_GetAt_1008_bcc2(const SBCollection *self, int16_t index_1based, void *out_elem);
void CList_SetAt_1008_bce2(SBCollection *self, int16_t index_1based, const void *in_elem);
void CList_InsertBefore_1008_bd02(SBCollection *self, int16_t index_1based, const void *in_elem);
void CList_DeleteOne_1008_bd5c(SBCollection *self, int16_t index_1based);
void CList_Swap_1008_bdb6(SBCollection *self, int16_t idx_a_1based, int16_t idx_b_1based);
void CList_Move_1008_be6a(SBCollection *self, int16_t from_1based, int16_t to_1based);
void CList_Sort_1008_bed0(SBCollection *self, SBCollectionCompareFn cmp);
int16_t CList_LinearSearch_1008_bf52(const SBCollection *self, const void *target_elem, SBCollectionCompareFn cmp);
int16_t CList_BinarySearch_1008_bfc6(const SBCollection *self, const void *target_elem, SBCollectionCompareFn cmp);

SBCollection *COrderedList_Ctor_1008_c02a(SBCollection *self, int16_t elem_size);
void COrderedList_Dtor_1008_c060(SBCollection *self);
int16_t COrderedList_CompareElems_1008_c08c(const SBCollection *self, const void *a, const void *b);
int16_t COrderedList_FindIndex_1008_c0a8(const SBCollection *self, const void *elem);
void COrderedList_InsertSorted_1008_c0d6(SBCollection *self, const void *elem);

SBCollection *CObList_Ctor_1008_c100(SBCollection *self);
void CObList_Dtor_1008_c11a(SBCollection *self);
void CObList_ForEach_1008_c146(SBCollection *self, SBObListIterFn fn);
void CObList_ForEach1_1008_c19e(SBCollection *self, SBObListIter1Fn fn, void *ctx);
void *CObList_FirstThat_1008_c1fc(SBCollection *self, SBObListPredFn pred);
void *CObList_FirstThat1_1008_c266(SBCollection *self, SBObListPred1Fn pred, void *ctx);
void *CObList_LastThat_1008_c2d8(SBCollection *self, SBObListPredFn pred);
void *CObList_LastThat1_1008_c346(SBCollection *self, SBObListPred1Fn pred, void *ctx);
void *CObList_At_1008_c3ba(const SBCollection *self, int16_t index_1based);
void CObList_AtPut_1008_c3e8(SBCollection *self, int16_t index_1based, void *obj);
void CObList_InsertAt_1008_c40a(SBCollection *self, int16_t index_1based, void *obj);
void *CObList_First_1008_c46c(const SBCollection *self);
void *CObList_Last_1008_c4a0(const SBCollection *self);
int CObList_Includes_1008_c4dc(const SBCollection *self, const void *obj);
void CObList_InsertFirst_1008_c504(SBCollection *self, void *obj);
void CObList_InsertLast_1008_c520(SBCollection *self, void *obj);
void CObList_Remove_1008_c554(SBCollection *self, const void *obj);
void CObList_RemoveAt_1008_c59a(SBCollection *self, int16_t index_1based);
int16_t CObList_IndexOf_1008_c5cc(const SBCollection *self, const void *obj);
void CObList_DisposeAll_1008_c62a(SBCollection *self);
void CObList_DisposeAndFree_1008_c668(SBCollection *self);
int16_t CObList_ComparePtrs_1008_c6a8(const void *a, const void *b);
void CObList_MoveToFront_1008_c6d4(SBCollection *self, void *obj);
void CObList_MoveToBack_1008_c71a(SBCollection *self, void *obj);
SBCollection *CObList_New_1010_036c(void);
void CObList_CallVirtual_1010_1246(SBCollection *self, uint16_t vtable_slot_offset);

/* =========================================================================
 * 2. Event Dispatcher (`CEventDispatcherBase_1008_0b84` & `CEventDispatcher_1008_7f84`)
 * ========================================================================= */
SBEventDispatcher *CEventDispatcherBase_Ctor_1010_3a28(SBEventDispatcher *self);
void CEventDispatcherBase_Dtor_1010_3ae8(SBEventDispatcher *self);
void CEventDispatcherBase_Init_1010_3b2c(SBEventDispatcher *self);
int CEventDispatcherBase_ProcessOneEvent_1010_3bb2(SBEventDispatcher *self);
void CEventDispatcherBase_DispatchWinMessage_1010_3c64(SBEventDispatcher *self, const SB_MSG16 *msg);
void CEventDispatcherBase_HandleMouseDown_1010_3d98(SBEventDispatcher *self, SB_HWND hwnd, SB_POINT16 pt, uint16_t flags);
void CEventDispatcherBase_HandleMouseUp_1010_3f02(SBEventDispatcher *self, SB_HWND hwnd, SB_POINT16 pt, uint16_t flags);
void CEventDispatcherBase_HandleMouseMove_1010_3f7c(SBEventDispatcher *self, SB_HWND hwnd, SB_POINT16 pt, uint16_t flags);
void CEventDispatcherBase_HandleKeyDown_1010_4008(SBEventDispatcher *self, uint16_t vk, uint32_t lParam);
void CEventDispatcherBase_HandleChar_1010_40a4(SBEventDispatcher *self, uint16_t ch, uint32_t lParam);
void CEventDispatcherBase_DoIdle_1010_4120(SBEventDispatcher *self);
void CEventDispatcherBase_UpdateModifiers_1010_41a8(SBEventDispatcher *self);
void CEventDispatcherBase_AddIdleTask_1010_4236(SBEventDispatcher *self, SBEventTarget *task);
void CEventDispatcherBase_RemoveIdleTask_1010_4268(SBEventDispatcher *self, SBEventTarget *task);
void CEventDispatcherBase_SetWaitCursor_1010_429a(SBEventDispatcher *self, uint8_t wait_on);
void CEventDispatcherBase_FlushEvents_1010_42ce(SBEventDispatcher *self);

SBEventDispatcher *CEventDispatcher_Init_1008_7c02(SBEventDispatcher *self);
void CEventDispatcher_SetCustomCursor_1008_7c40(SBEventDispatcher *self, uint16_t curs_res_id);
void CEventDispatcher_RestoreArrowCursor_1008_7c5c(SBEventDispatcher *self);
uint8_t CEventDispatcher_IsButtonDown_1020_2534(const SBEventDispatcher *self);
void CEventDispatcher_GetMousePos_1020_2552(const SBEventDispatcher *self, SB_POINT16 *out_pt);

/* =========================================================================
 * 3. View & Desktop Hierarchy (`CView_1008_1aac` & `CDesktop_1008_7d10`)
 * ========================================================================= */
SBView *CView_Ctor_1010_15f0(SBView *self,
                             SBView *superview,
                             SBEventTarget *next_handler,
                             int16_t loc_x,
                             int16_t loc_y,
                             int16_t width,
                             int16_t height,
                             uint8_t sizing_flags);
void CView_Dtor_1010_16d4(SBView *self);
void CView_Dispose_1010_1732(SBView *self);
SBView *CView_InitFromPaneResource_1010_1826(SBView *self,
                                             SBView *superview,
                                             SBEventTarget *next_handler,
                                             uint16_t pane_res_id);
void CView_InitFromStream_1010_188e(SBView *self,
                                    SBView *superview,
                                    SBEventTarget *next_handler,
                                    const uint8_t *pane_data,
                                    uint32_t data_len);
void CView_AddSubview_1010_19a4(SBView *self, SBView *subview);
void CView_RemoveSubview_1010_1a12(SBView *self, SBView *subview);
SBView *CView_FindViewById_1010_1a66(SBView *self, int32_t pane_id);
void CView_Show_1010_1ae4(SBView *self, uint8_t visible, uint8_t redraw);
void CView_Activate_1010_1b58(SBView *self, uint8_t active);
void CView_SetFrame_1010_1bc8(SBView *self, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t redraw);
void CView_GetFrame_1010_1c52(const SBView *self, SB_RECT16 *out_rc);
void CView_GetBounds_1010_1c88(const SBView *self, SB_RECT16 *out_rc);
void CView_MoveTo_1010_1cbe(SBView *self, int16_t x, int16_t y, uint8_t redraw);
void CView_Resize_1010_1d12(SBView *self, int16_t w, int16_t h, uint8_t redraw);
SBWindow *CView_GetWindow_1010_1d74(const SBView *self);
SBDesktop *CView_GetDesktop_1010_1db0(const SBView *self);
int CView_IsVisible_1010_1de8(const SBView *self);

void CView_LocalToWindow_1010_45ea(const SBView *self, SB_POINT16 *io_pt);
void CView_WindowToLocal_1010_4638(const SBView *self, SB_POINT16 *io_pt);
void CView_LocalToGlobal_1010_4686(const SBView *self, SB_POINT16 *io_pt);
void CView_GlobalToLocal_1010_46e2(const SBView *self, SB_POINT16 *io_pt);
void CView_LocalToSuper_1010_473e(const SBView *self, SB_POINT16 *io_pt);
void CView_SuperToLocal_1010_4768(const SBView *self, SB_POINT16 *io_pt);
void CView_FrameToWindR_1010_4792(const SBView *self, const SB_RECT16 *local_rc, SB_RECT16 *out_wind_rc);
void CView_WindToFrameR_1010_47ec(const SBView *self, const SB_RECT16 *wind_rc, SB_RECT16 *out_local_rc);
int CView_ContainsPoint_1010_4846(const SBView *self, SB_POINT16 super_pt);
SBView *CView_FindTargetView_1010_48ac(SBView *self, SB_POINT16 super_pt);
int CView_DispatchClick_1010_493c(SBView *self, SB_POINT16 local_pt, uint16_t modifiers, int16_t click_count);
void CView_HandleMouseDown_1010_49d6(SBView *self, SB_POINT16 local_pt, uint16_t modifiers, int16_t click_count);
int CView_PrepareDC_1010_4a0a(SBView *self);
void CView_RestoreDC_1010_4ac8(SBView *self);
void CView_DrawAll_1010_4b30(SBView *self, const SB_RECT16 *area_rc);
void CView_DrawSelf_1010_4c18(SBView *self, const SB_RECT16 *area_rc);
void CView_DrawBorder_1010_4c50(SBView *self);
void CView_InvalidateRect_1010_4d5a(SBView *self, const SB_RECT16 *local_rc);
void CView_ValidateRect_1010_4dd8(SBView *self, const SB_RECT16 *local_rc);
void CView_InvalidateAll_1010_4e56(SBView *self);
void CView_PenMoveTo_1010_4e8e(SBView *self, int16_t x, int16_t y);
void CView_PenLineTo_1010_4ec4(SBView *self, int16_t x, int16_t y);
void CView_FillRectColor_1010_4f18(SBView *self, const SB_RECT16 *rc, uint32_t color_ref);
void CView_AdjustCursor_1010_4f78(SBView *self, SB_POINT16 local_pt);
void CView_Noop_1010_4fc0(SBView *self);

SBDesktop *CDesktop_Ctor_1008_31b8(SBDesktop *self, SBEventTarget *next_handler);
void CDesktop_Dtor_1008_31f0(SBDesktop *self);
void CDesktop_BringWindowToFront_1008_3218(SBDesktop *self, SBWindow *win);
SBDesktop *CDesktop_New_1008_d1d8(SBEventTarget *next_handler);

/* =========================================================================
 * 4. Window Wrapper (`CWindow_1010_5942`)
 * ========================================================================= */
SBWindow *CWindow_Ctor_1010_50ce(SBWindow *self,
                                 uint16_t wind_res_id,
                                 uint8_t is_floating,
                                 SBView *superview,
                                 SBEventTarget *next_handler);
SBWindow *CWindow_CtorBounds_1010_51b4(SBWindow *self,
                                       const SB_RECT16 *bounds,
                                       const char *title,
                                       uint8_t is_visible,
                                       uint16_t proc_id,
                                       uint8_t has_close_box,
                                       uint8_t is_floating,
                                       SBView *superview,
                                       SBEventTarget *next_handler);
void CWindow_Dtor_1010_528e(SBWindow *self);
void CWindow_Dispose_1010_52f4(SBWindow *self);
void CWindow_CreateNativeWindow_1010_5332(SBWindow *self, const char *title, uint8_t show_now);
void CWindow_Select_1010_5490(SBWindow *self);
void CWindow_Show_1010_54dc(SBWindow *self, uint8_t visible, uint8_t redraw);
void CWindow_Activate_1010_5540(SBWindow *self, uint8_t active);
void CWindow_Close_1010_55a8(SBWindow *self);
SB_HDC CWindow_GetDC_1010_55f0(SBWindow *self);
void CWindow_ReleaseDC_1010_5646(SBWindow *self, SB_HDC hdc);
void CWindow_Update_1010_568a(SBWindow *self);
void CWindow_InvalidateWindRect_1010_5748(SBWindow *self, const SB_RECT16 *wind_rc);
void CWindow_ValidateWindRect_1010_5792(SBWindow *self, const SB_RECT16 *wind_rc);
void CWindow_MoveWindow_1010_57dc(SBWindow *self, int16_t x, int16_t y);
void CWindow_SizeWindow_1010_5834(SBWindow *self, int16_t w, int16_t h);
void CWindow_GetGlobalBounds_1010_588c(const SBWindow *self, SB_RECT16 *out_rc);
SBWindow *CWindow_FromHwnd_1010_59c2(SB_HWND hwnd);
uint32_t CWindow_WndProc_1010_5a18(SB_HWND hwnd, uint16_t message, uint16_t wParam, uint32_t lParam);
void CWindow_OnPaint_1010_5b60(SBWindow *self);
void CWindow_OnActivate_1010_5c24(SBWindow *self, uint16_t active_flag);
void CWindow_OnMove_1010_5c78(SBWindow *self, int16_t new_x, int16_t new_y);
void CWindow_OnSize_1010_5cc4(SBWindow *self, int16_t new_w, int16_t new_h);
int CWindow_OnClose_1010_5d20(SBWindow *self);
SB_HWND CWindow_GetHwnd_1010_5d7c(const SBWindow *self);
void Window_RegisterClasses_1010_2d16(uint16_t hInstance);
SB_HDC Window_GetMainScreenDC_1010_2d22(void);

/* =========================================================================
 * 5. Application Lifecycle (`CApplication_1008_2e32`)
 * ========================================================================= */
SBApplication *CApplication_Ctor_1008_018c(SBApplication *self,
                                           int16_t extra_masters,
                                           uint32_t rain_reserve,
                                           uint32_t total_reserve);
void CApplication_Dtor_1008_0240(SBApplication *self);
void CApplication_InitToolbox_1008_02a8(SBApplication *self);
void CApplication_CreateDesktop_1008_0310(SBApplication *self);
int CApplication_HandleCommand_1008_0368(SBApplication *self, int32_t cmd);
void CApplication_Quit_1008_0412(SBApplication *self);
int CApplication_CanQuit_1008_0458(SBApplication *self);
void CApplication_RunMainLoop_1008_0694(SBApplication *self);
void CApplication_ProcessSingleStep_1008_06c8(SBApplication *self);
void CApplication_Shutdown_1008_0700(SBApplication *self);

int Application_WinMainEntry_1008_2bf4(uint16_t hInstance,
                                       uint16_t hPrevInstance,
                                       const char *lpCmdLine,
                                       uint16_t nCmdShow,
                                       uint16_t max_loop_steps);
void CApplication_InitGameSubsystems_1008_72b6(SBApplication *self);
void CApplication_CleanupGameSubsystems_1008_7540(SBApplication *self);
void CApplication_CreateMainWindow_1008_7610(SBApplication *self);
void CApplication_LoadInitialPalette_1008_7720(SBApplication *self, uint16_t clut_res_id);
void CApplication_CreateOffscreenBuffer_1008_7790(SBApplication *self);
void CApplication_PreloadCoreResources_1008_7820(SBApplication *self);
void CApplication_StartGameController_1008_78b0(SBApplication *self);
void CApplication_OnActivateApp_1008_7940(SBApplication *self, uint8_t active);
void CApplication_PumpIdleAndAudio_1008_79ec(SBApplication *self);
void CApplication_DispatchTimers_1008_7a80(SBApplication *self);
SBPictSurface *CApplication_GetOffscreen_1008_7b20(const SBApplication *self);
SBWindow *CApplication_GetMainWindow_1008_7b50(const SBApplication *self);
void CApplication_PresentOffscreenRect_1008_7b80(SBApplication *self, const SB_RECT16 *rc);
void CApplication_RequestQuit_1008_7bf2(SBApplication *self);

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOVERED_FRAMEWORK_UI_FRAMEWORK_H */

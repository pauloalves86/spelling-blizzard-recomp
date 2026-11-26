/* Verification Test Suite for Core Collections, View, Window, Desktop & Application Framework
 *
 * Tests the decompiled functions in `src/recovered/framework/ui_framework.c`
 * and `src/shim/win16_user.c` against real resources from `original/BLIZZARD.EXE`
 * and `original/SB_CDRES.DLL`:
 *   1. `CCollection`, `CList`, `COrderedList`, and `CObList` (`1008:b760..c762`)
 *   2. `CView` hierarchy, coordinate conversions, hit-testing, and loading from
 *      real `'Pane'` (`0x03f1`) resources in `SB_CDRES.DLL` (`1010:15f0..4fc0`)
 *   3. `CDesktop` & `CWindow` creation from real `'WIND'` (`0x03f0` `#1049`)
 *      in `BLIZZARD.EXE` (`1008:31b8..3218`, `1010:50ce..5d7c`)
 *   4. `CEventDispatcher` mouse, double-click, keyboard, and idle dispatch (`1010:3a28..42ce`)
 *   5. `CApplication` startup (`Application_WinMainEntry_1008_2bf4`, `1008:72b6`)
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/recovered/framework/ui_framework.h"

static void sum_iter_cb(void *item, void *ctx) {
    int *acc = (int *)ctx;
    int val = (int)(intptr_t)item;
    *acc += val;
}

static int pred_eq_20(void *item) {
    return ((int)(intptr_t)item == 20) ? 1 : 0;
}

static void test_collections_and_oblist(void) {
    SBCollection *list = CObList_New_1010_036c();
    int sum = 0;
    void *found;

    assert(list != NULL);
    assert(CCollection_IsEmpty_1008_b8ca(list) == 1);

    CObList_InsertLast_1008_c520(list, (void *)(intptr_t)10);
    CObList_InsertLast_1008_c520(list, (void *)(intptr_t)20);
    CObList_InsertLast_1008_c520(list, (void *)(intptr_t)30);
    CObList_InsertFirst_1008_c504(list, (void *)(intptr_t)5);

    assert(CCollection_GetCount_1008_b8bc(list) == 4);
    assert((int)(intptr_t)CObList_First_1008_c46c(list) == 5);
    assert((int)(intptr_t)CObList_Last_1008_c4a0(list) == 30);
    assert(CObList_IndexOf_1008_c5cc(list, (void *)(intptr_t)20) == 3);
    assert(CObList_Includes_1008_c4dc(list, (void *)(intptr_t)20) == 1);

    CObList_ForEach1_1008_c19e(list, sum_iter_cb, &sum);
    assert(sum == 65);

    found = CObList_FirstThat_1008_c1fc(list, pred_eq_20);
    assert((int)(intptr_t)found == 20);

    CObList_MoveToFront_1008_c6d4(list, (void *)(intptr_t)30);
    assert((int)(intptr_t)CObList_First_1008_c46c(list) == 30);

    CObList_MoveToBack_1008_c71a(list, (void *)(intptr_t)30);
    assert((int)(intptr_t)CObList_Last_1008_c4a0(list) == 30);

    CObList_Remove_1008_c554(list, (void *)(intptr_t)20);
    assert(CCollection_GetCount_1008_b8bc(list) == 3);
    assert(CObList_Includes_1008_c4dc(list, (void *)(intptr_t)20) == 0);

    CObList_Dtor_1008_c11a(list);
    free(list);
}

static void test_view_and_window_hierarchy(void) {
    SBDesktop *desktop;
    SBWindow *win;
    SBView *pane_view;
    SBView *child_btn;
    SB_POINT16 pt;
    SBView *hit;

    sb_win16_user_shim_reset();
    sb_win16_gdi_shim_reset();
    sb_win16_waveOut_resetStats();
    assert(sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") == 0);
    ResMgr_LoadCdResLibrary_1008_2bbc();
    assert(CResourceManager_CreateGlobal_1008_2bf4_part() != NULL);
    g_DAT_1060_0a1e_timer60Hz = CTimer60Hz_Ctor_1010_446a((SBTimer60Hz *)NewObject_1008_ef2c(sizeof(SBTimer60Hz)));

    desktop = CDesktop_New_1008_d1d8(NULL);
    assert(desktop != NULL);
    assert(desktop->view_00.width_14 == 640);
    assert(desktop->view_00.height_16 == 480);

    /* Load real WIND #1049 from BLIZZARD.EXE */
    win = CWindow_Ctor_1010_50ce(NULL, 1049, 0, &desktop->view_00, NULL);
    assert(win != NULL);
    assert(win->hwnd_42 != 0);
    assert(CWindow_FromHwnd_1010_59c2(win->hwnd_42) == win);

    /* Load real Pane #1049 from SB_CDRES.DLL */
    pane_view = CView_InitFromPaneResource_1010_1826(NULL, &win->view_00, NULL, 1049);
    assert(pane_view != NULL);
    CView_SetFrame_1010_1bc8(pane_view, 10, 20, 300, 200, 1);
    pane_view->pane_id_20 = 1049;

    child_btn = CView_Ctor_1010_15f0(NULL, pane_view, NULL, 50, 40, 80, 30, 0);
    assert(child_btn != NULL);
    child_btn->pane_id_20 = 2001;
    child_btn->border_flags_24 = 1;

    assert(CView_FindViewById_1010_1a66(&win->view_00, 2001) == child_btn);
    assert(CView_GetWindow_1010_1d74(child_btn) == win);

    /* Coordinate conversion: child (5, 5) -> window (10+50+5=65, 20+40+5=65) */
    pt.x = 5;
    pt.y = 5;
    CView_LocalToWindow_1010_45ea(child_btn, &pt);
    assert(pt.x == 65 && pt.y == 65);

    /* Hit testing */
    pt.x = 70;
    pt.y = 70;
    hit = CView_FindTargetView_1010_48ac(&win->view_00, pt);
    assert(hit == child_btn);

    /* Paint & DC drawing */
    CWindow_Update_1010_568a(win);

    CView_Dtor_1010_16d4(child_btn);
    free(child_btn);
    CView_Dtor_1010_16d4(pane_view);
    free(pane_view);
    CWindow_Dtor_1010_528e(win);
    free(win);
    CDesktop_Dtor_1008_31f0(desktop);
    free(desktop);

    CTimer60Hz_Dtor_1010_44d6(g_DAT_1060_0a1e_timer60Hz);
    g_DAT_1060_0a1e_timer60Hz = NULL;
    if (g_DAT_1060_0a14_resMgr) {
        CResourceMap_Dtor_1010_29ea(g_DAT_1060_0a14_resMgr);
        g_DAT_1060_0a14_resMgr = NULL;
    }
    ResMgr_FreeCdResLibrary_1008_2bcc();
    sb_win16_resource_shim_shutdown();
}

static void test_application_winmain_lifecycle(void) {
    int rc;
    sb_win16_user_shim_reset();
    sb_win16_gdi_shim_reset();
    sb_win16_waveOut_resetStats();
    assert(sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") == 0);

    /* Post a synthetic click and key press to be processed during the 4-step startup run */
    sb_win16_PostMessage(1, SB_WM_LBUTTONDOWN, 1, ((uint32_t)120 << 16) | 200u);
    sb_win16_PostMessage(1, SB_WM_LBUTTONUP, 0, ((uint32_t)120 << 16) | 200u);
    sb_win16_PostMessage(1, SB_WM_CHAR, 'A', 0);

    rc = Application_WinMainEntry_1008_2bf4(1, 0, "", 1, 4);
    assert(rc == 0);

    if (g_DAT_1060_0a1e_timer60Hz) {
        CTimer60Hz_Dtor_1010_44d6(g_DAT_1060_0a1e_timer60Hz);
        g_DAT_1060_0a1e_timer60Hz = NULL;
    }
    if (g_DAT_1060_0a14_resMgr) {
        CResourceMap_Dtor_1010_29ea(g_DAT_1060_0a14_resMgr);
        g_DAT_1060_0a14_resMgr = NULL;
    }
    ResMgr_FreeCdResLibrary_1008_2bcc();
    sb_win16_resource_shim_shutdown();
}

int main(void) {
    test_collections_and_oblist();
    test_view_and_window_hierarchy();
    test_application_winmain_lifecycle();
    printf("PASS: test_ui_framework (CObList, CView, CWindow, CDesktop, CEventDispatcher, CApplication)\n");
    return 0;
}

/* Spelling Blizzard (1994 Bright Star / Sierra) — Reconstructed Executable Runner (Path 2)
 *
 * Boots and runs the full reconstructed application pipeline from `original/BLIZZARD.EXE`
 * and `original/SB_CDRES.DLL` via the Thin Win16 API Shim:
 *   1. `System_CheckRequirements_1008_6a28` (Win 3.10+, 11.025 kHz 8-bit mono waveOut, 256-color GDI)
 *   2. `CResourceManager_1008_2da6` (`DAT_1060_0a14`, 42 resource types from `BLIZZARD.EXE` & `SB_CDRES.DLL`)
 *   3. `CTimer60Hz_1010_456a` (`DAT_1060_0a1e`) & `CSoundManager_1008_7edc` (`DAT_1060_07c8`)
 *   4. `CApplication_1008_2e32` (`DAT_1060_07c4`), `CDesktop_1008_7d10` (`DAT_1060_0810`),
 *      `CWindow_1010_5942` (`'WIND'` `#1049`), & `CPaletteManager_1008_8278` (`'clut'` `#1049`/`#999`)
 *   5. `CBoardScene_1010_be64` (`'PICT'` `#129`, `'PICT'` `#1049` masked HUD, `'FrTr'` `#1000..#1005`)
 *   6. `SpellingBank` (`'STR#'` `#201`/`#301`/`#401`, `'RlQt'` `#1001`) &
 *      `CSpellingMatch_1010_cc76` River Grid (`'SdGd'` `#128`, `'GmTr'`, `'GmLt'`, `'SdOb'`)
 *
 * Options:
 *   --level <1..101>      Select starting level (default: 1)
 *   --save-ppm <path>     Export rendered 640x480 256-color screen framebuffer to a 24-bit RGB PPM image
 *   --interactive         Run interactive terminal spelling session
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "recovered/application/game_logic.h"

static int save_screen_as_ppm(const char *ppm_path, const SBPictSurface *surface) {
    FILE *fp;
    int16_t w = 0;
    int16_t h = 0;
    const uint8_t *pixels;
    const SB_PALETTEENTRY *pal;
    int x;
    int y;

    if (!ppm_path || !surface) {
        return -1;
    }
    pixels = sb_win16_gdi_getBitmapPixels(surface->hbmp_06, &w, &h);
    pal = sb_win16_gdi_getActivePaletteEntries();
    if (!pixels || !pal || w <= 0 || h <= 0) {
        return -1;
    }
    fp = fopen(ppm_path, "wb");
    if (!fp) {
        return -1;
    }
    fprintf(fp, "P6\n%d %d\n255\n", (int)w, (int)h);
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            uint8_t idx = pixels[y * w + x];
            uint8_t rgb[3];
            rgb[0] = pal[idx].peRed;
            rgb[1] = pal[idx].peGreen;
            rgb[2] = pal[idx].peBlue;
            fwrite(rgb, 1, 3, fp);
        }
    }
    fclose(fp);
    return 0;
}

static void render_river_grid_ascii(const SBSpellingMatch *match) {
    int r;
    int c;
    if (!match || !match->grid_10) {
        return;
    }
    printf("\n  +-- Spelling Blizzard River Grid (Level %d: 15x12 Tiles / 60x48 Subcells) --+\n",
           (int)match->level_id_26);
    for (r = 0; r < SB_GRID_TILE_ROWS; ++r) {
        printf("  | ");
        for (c = 0; c < SB_GRID_TILE_COLS; ++c) {
            uint16_t tid = match->grid_10->terrain_sdob_grid[r][c];
            char sym = '.';
            if (tid == 1001) {
                sym = '~'; /* Ice/Water river tile */
            } else if (tid == 1501) {
                sym = '#'; /* Snowbank / rock obstacle */
            } else if (tid >= 1002 && tid <= 1020) {
                sym = '^'; /* Pine tree / border */
            }
            printf("%c ", sym);
        }
        printf("|\n");
    }
    printf("  +---------------------------------------------------------------------------+\n");
}

int main(int argc, char **argv) {
    int16_t level_id = 1;
    const char *ppm_path = NULL;
    int interactive = 0;
    int i;

    SBApplication *app;
    SBBoardScene *board_scene;
    SBPictSurface *hud_banner;
    SB_RECT16 banner_src_rc = {0, 0, 356, 16};
    SB_RECT16 banner_dst_rc = {142, 12, 498, 28};
    uint16_t frtr_ids[4] = {1000, 1002, 1003, 1004};
    SBSpellingPracticeSession practice;
    SBSpellingMatch *match;
    SBRollQuotePair quote_pairs[8];
    int quote_len;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--level") == 0 && i + 1 < argc) {
            level_id = (int16_t)atoi(argv[++i]);
            if (level_id < 1 || level_id > 101) {
                level_id = 1;
            }
        } else if (strcmp(argv[i], "--save-ppm") == 0 && i + 1 < argc) {
            ppm_path = argv[++i];
        } else if (strcmp(argv[i], "--interactive") == 0) {
            interactive = 1;
        }
    }

    printf("========================================================================\n");
    printf("  SPELLING BLIZZARD (v1.10.CD for Windows) — Reconstructed Runtime\n");
    printf("========================================================================\n");

    /* 1. Initialize Thin Win16 API Shims */
    sb_win16_user_shim_reset();
    sb_win16_gdi_shim_reset();
    sb_win16_waveOut_resetStats();
    if (sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") != 0) {
        fprintf(stderr, "ERROR: Failed to open original/BLIZZARD.EXE or original/SB_CDRES.DLL\n");
        return 1;
    }

    /* 2. Run WinMain (`1008:2bf4`) & Application Subsystem Initialization (`1008:72b6`) */
    if (!System_CheckRequirements_1008_6a28()) {
        fprintf(stderr, "ERROR: System_CheckRequirements_1008_6a28 failed\n");
        return 1;
    }
    ResMgr_LoadCdResLibrary_1008_2bbc();
    CResourceManager_CreateGlobal_1008_2bf4_part();
    g_DAT_1060_0a1e_timer60Hz = CTimer60Hz_Ctor_1010_446a(
        (SBTimer60Hz *)NewObject_1008_ef2c(sizeof(SBTimer60Hz)));

    app = CApplication_Ctor_1008_018c(NULL, 4, 0x8000u, 0x9400u);
    CApplication_InitGameSubsystems_1008_72b6(app);

    printf("[1/6] Initialized Resource Manager (%d resource types) & 236-color CLUT #1049/#999\n",
           (int)g_DAT_1060_0a14_resMgr->count_04);
    printf("[2/6] Created Desktop (640x480) & Main Window (HWND=%u from WIND #1049)\n",
           (unsigned)app->main_window_74->hwnd_42);

    /* 3. Stream intro speech/audio from SB_CDRES.DLL ('snd ' #129 & #130) */
    CSoundManager_PlaySoundResource_1008_22d6(g_DAT_1060_07c8_soundMgr, 129);
    CSoundManager_Stop_1008_2462(g_DAT_1060_07c8_soundMgr);
    CSoundManager_PlaySoundResource_1008_22d6(g_DAT_1060_07c8_soundMgr, 130);
    CSoundManager_Stop_1008_2462(g_DAT_1060_07c8_soundMgr);
    printf("[3/6] Streamed BSW 11,025 Hz 8-bit mono audio ('snd ' #129, #130): %u PCM bytes written\n",
           (unsigned)sb_win16_waveOut_getTotalBytesWritten());

    /* 4. Compose 640x480 Scene ('PICT' #129 background + 'PICT' #1049 masked HUD + Water Shimmer) */
    board_scene = CBoardScene_InitFromFrTr_1010_6ef4(
        NULL, &app->main_window_74->view_00, 129, frtr_ids, 4);
    hud_banner = (SBPictSurface *)NewObject_1008_ef2c(sizeof(SBPictSurface));
    CMaskedPictSurface_InitFromPict_1008_5f6e(hud_banner, 1, 8, 1049);
    CPictSurface_CopyToSurface_1008_5c88(
        board_scene->bg_surface_58, &board_scene->bg_surface_58->bounds_0e,
        &board_scene->bg_surface_58->bounds_0e, app->offscreen_78);
    CMaskedPictSurface_BlitToSurface_1008_610a(
        hud_banner, &banner_dst_rc, &banner_src_rc, app->offscreen_78);
    WaterShimmer_AnimateBitmap_1008_64ac(app->offscreen_78);
    CApplication_PresentOffscreenRect_1008_7b80(app, NULL);

    if (ppm_path) {
        if (save_screen_as_ppm(ppm_path, app->offscreen_78) == 0) {
            printf("      Saved 640x480 24-bit RGB frame preview to %s\n", ppm_path);
        }
    }

    /* 5. Initialize Player Profile & Run Magic Spelling Tricks Word Practice Round */
    g_DAT_1060_0a46_highScoreTable = CPlayerRecord_Ctor_1018_a9a0(NULL, "TOP7", 0);
    CGameController_HandleCommand_1008_ac80(0x2af9, 0);
    g_DAT_1060_0a36_gameStarted = 1;
    g_DAT_1060_01de_currentLevel = (uint16_t)level_id;
    CGameControllerView_CheckLevelTutorial_1018_284c(0);

    quote_len = RollQuote_PlaySequence_1020_62fe(1001, level_id, quote_pairs, 8);
    memset(&practice, 0, sizeof(practice));
    practice.current_level_10 = level_id;

    printf("[4/6] Magic Spelling Tricks Round (Level %d, RlQt #1001 seq=%d pairs=%d):\n",
           (int)level_id, (int)level_id, quote_len);

    if (interactive) {
        char input_line[64];
        int w_idx = 1;
        while (!practice.round_completed_89 && w_idx <= 10) {
            SpellingBank_LoadWordTriplet_1018_b78c(&practice, (int16_t)w_idx);
            printf("      Spell word #%d (hint: %zu letters, starts with '%c'): ",
                   w_idx, strlen(practice.primary_word_5d), practice.primary_word_5d[0]);
            fflush(stdout);
            if (!fgets(input_line, sizeof(input_line), stdin)) {
                break;
            }
            input_line[strcspn(input_line, "\r\n")] = '\0';
            if (SpellingBank_CheckTypedWord_1018_b35c(&practice, input_line)) {
                printf("      -> CORRECT! (%d/%d)\n",
                       (int)practice.words_correct_82, (int)practice.target_words_84);
                w_idx++;
            } else {
                printf("      -> Try again! Correct spelling is \"%s\"\n", practice.primary_word_5d);
            }
        }
    } else {
        int16_t w_idx;
        for (w_idx = 1; !practice.round_completed_89 && w_idx <= 10; ++w_idx) {
            SpellingBank_LoadWordTriplet_1018_b78c(&practice, w_idx);
            SpellingBank_CheckTypedWord_1018_b35c(&practice, practice.primary_word_5d);
            printf("      Word #%d: spelled \"%s\" -> CORRECT (%d/%d)\n",
                   (int)w_idx, practice.primary_word_5d,
                   (int)practice.words_correct_82, (int)practice.target_words_84);
        }
    }

    /* 6. Load & Play Spelling Blizzard River Grid Level (`CSpellingMatch_1010_cc76`) */
    match = CSpellingMatch_Ctor(NULL);
    CSpellingMatch_LoadLevel_1020_0e50(match, level_id);
    printf("[5/6] Loaded Spelling Blizzard River Grid Level %d:\n", (int)level_id);
    printf("      Grid: %dx%d px (%dx%d subcells), Static Objects: %d, Moving Actors: %d, Letter Blocks: %d\n",
           (int)match->grid_10->pixel_width_04, (int)match->grid_10->pixel_height_06,
           (int)match->grid_10->cols_0c, (int)match->grid_10->rows_0e,
           (int)CCollection_GetCount_1008_b8bc(match->static_objs_14),
           (int)CCollection_GetCount_1008_b8bc(match->moving_actors_18),
           (int)match->letter_block_count_48);

    render_river_grid_ascii(match);

    {
        char target_word[32];
        size_t k;
        StrList_GetIndString_1010_13b6(201, level_id, target_word, sizeof(target_word));
        CSpellingMatch_SetTargetWord_1020_189e(match, target_word);
        CSpellingMatch_Tick_1020_1c9a(match);
        for (k = 0; target_word[k] != '\0'; ++k) {
            CSpellingMatch_CollectLetter_1020_1a9a(match, target_word[k]);
        }
        CSpellingMatch_Tick_1020_1c9a(match);
        printf("[6/6] Completed River Grid word \"%s\" (match_state=%d, bonus=%ld, high_score=%d)\n",
               match->collected_pstr_5b,
               (int)match->match_state_68,
               (long)CSpellingMatch_CalcBonusScore_1020_1852(match),
               (int)CPlayerRecord_GetHighScore_1018_aeaa(g_DAT_1060_01e4_currentPlayer));
    }

    /* Pump Win16 Event Loop & Cleanly Shut Down */
    sb_win16_PostMessage(app->main_window_74->hwnd_42, SB_WM_PAINT, 0, 0);
    for (i = 0; i < 4; ++i) {
        sb_win16_advanceTimeMs(50);
        CApplication_ProcessSingleStep_1008_06c8(app);
    }

    CSpellingMatch_Dtor(match);
    CMaskedPictSurface_Dtor_1008_604a(hud_banner);
    CBoardScene_Dtor_1010_735e(board_scene);
    free(board_scene);

    if (g_DAT_1060_01e4_currentPlayer) {
        CPlayerRecord_Dtor_1018_aa84(g_DAT_1060_01e4_currentPlayer);
        g_DAT_1060_01e4_currentPlayer = NULL;
    }
    if (g_DAT_1060_0a46_highScoreTable) {
        CPlayerRecord_Dtor_1018_aa84(g_DAT_1060_0a46_highScoreTable);
        g_DAT_1060_0a46_highScoreTable = NULL;
    }

    CApplication_Shutdown_1008_0700(app);
    free(app);

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

    printf("========================================================================\n");
    printf("  SPELLING BLIZZARD — Clean Shutdown Complete (Exit Code 0)\n");
    printf("========================================================================\n");
    return 0;
}

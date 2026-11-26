/* Verification Test Suite for Spelling Blizzard Game Logic, Word Bank & River Grid Subsystems
 *
 * Tests `src/recovered/application/game_logic.c` against real assets from
 * `original/BLIZZARD.EXE` and `original/SB_CDRES.DLL`:
 *   1. `'STR#'` (`0x03ed`) word lists (`#201` primary words: "pastime", "innocent", "misspell", etc.,
 *      `#301` alternate 1, `#401` alternate 2, `#128..#133`) & Pascal string helpers (`1010:13b6..1504`).
 *   2. `CPlayerRecord_1008_2b04` top-7 high score table (`1018:a704`), word weight adjustment (`1018:adb8`),
 *      level badges (`1018:ae7e`, `1018:aec0`), and tutorial thresholds (`1018:aede`, `1018:af08`).
 *   3. `SpellingBank_LoadWordTriplet_1018_b78c` & `SpellingBank_CheckTypedWord_1018_b35c` full spelling
 *      practice round + `'RlQt'` (`0x03eb`) speech sequence decoding (`1020:62fe`).
 *   4. `CSpellingMatch_1010_cc76` River Grid level loader (`1020:0e50`):
 *      - `'SdGd'` (`0x0405` `#128`: 480x384 px, 60x48 subcells, 15x12 tiles)
 *      - `'GmTr'` (`0x0406` `#1001..#1005`: 180 terrain tiles per level)
 *      - `'GmLt'` (`0x0407` `#1001..#1005` & `#2001..#2005`: static letter blocks & moving demon actors)
 *      - `'SdOb'` (`0x0403` `#1001..#1027`: letter codes `'/'`, `'A'..'Z'`)
 *      - `CSpellingMatch_CollectLetter_1020_1a9a`: letter-by-letter spelling completion on the river grid!
 *   5. `CBoardScene_1010_be64` (`1010:6ef4`) loading real `'FrTr'` (`0x040c` `#1000..#1005`) & `'PICT'` `#129`.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/recovered/application/game_logic.h"

static void test_str_list_and_word_bank(void) {
    char word[64];
    SBRollQuotePair pairs[16];
    int pair_count;

    /* Test Pascal <-> C string in-place conversions (1010:14c2, 1010:1504) */
    strcpy(word, "blizzard");
    Str_CtoPstr_1010_14c2(word);
    assert((uint8_t)word[0] == 8);
    assert(memcmp(word + 1, "blizzard", 8) == 0);
    Str_PtoCstr_1010_1504(word);
    assert(strcmp(word, "blizzard") == 0);

    /* Load real spelling words from STR# #201 in SB_CDRES.DLL (1010:13b6) */
    assert(StrList_GetIndString_1010_13b6(201, 1, word, sizeof(word)) == 1);
    assert(strcmp(word, "pastime") == 0);
    assert(StrList_GetIndString_1010_13b6(201, 2, word, sizeof(word)) == 1);
    assert(strcmp(word, "innocent") == 0);
    assert(StrList_GetIndString_1010_13b6(201, 3, word, sizeof(word)) == 1);
    assert(strcmp(word, "misspell") == 0);
    assert(StrList_GetIndString_1010_13b6(201, 201, word, sizeof(word)) == 1);
    assert(strlen(word) > 0);

    /* Load real RlQt #1001 sequence 1 (1020:62fe) */
    pair_count = RollQuote_PlaySequence_1020_62fe(1001, 1, pairs, 16);
    assert(pair_count == 6);
    assert(pairs[0].tag_fourcc2 == 0x7268u); /* 'hr' little-endian */
    assert(pairs[0].item_id == 1);
}

static void test_player_profile_and_spelling_practice_round(void) {
    SBSpellingPracticeSession session;
    char cheat_buf[8] = {0};

    g_DAT_1060_0a46_highScoreTable = CPlayerRecord_Ctor_1018_a9a0(NULL, "TABLE", 0);
    assert(g_DAT_1060_0a46_highScoreTable != NULL);

    /* Initialize current player via 0x2af9 command (1008:ac80) */
    CGameController_HandleCommand_1008_ac80(0x2af9, 0);
    assert(g_DAT_1060_01e4_currentPlayer != NULL);
    assert(g_DAT_1060_01de_currentLevel == 1);
    assert(CGameControllerView_CheckLevelTutorial_1018_284c(0) == 1);
    assert(CGameControllerView_CheckLevelTutorial_1018_284c(0) == 0); /* already seen */

    g_DAT_1060_0a36_gameStarted = 1;
    g_DAT_1060_01de_currentLevel = 10;
    g_DAT_1060_0a52_levelCompleteBadgePending = 1;

    /* Run a 4-word spelling round at level 10 (target_words = 10/10 + 3 = 4) */
    memset(&session, 0, sizeof(session));
    session.current_level_10 = 10;

    /* Word 1: "pastime" — test misspelling first, then correct spelling */
    SpellingBank_LoadWordTriplet_1018_b78c(&session, 1);
    assert(strcmp(session.primary_word_5d, "pastime") == 0);
    assert(SpellingBank_CheckTypedWord_1018_b35c(&session, "pasttime") == 0);
    assert(g_DAT_1060_01e4_currentPlayer->nested_250->word_weights_08[0] == 140);
    assert(SpellingBank_CheckTypedWord_1018_b35c(&session, "PASTIME") == 1);
    assert(g_DAT_1060_01e4_currentPlayer->nested_250->word_weights_08[0] == 120);
    assert(session.words_correct_82 == 1);

    /* Word 2: "innocent" */
    SpellingBank_LoadWordTriplet_1018_b78c(&session, 2);
    assert(SpellingBank_CheckTypedWord_1018_b35c(&session, "innocent") == 1);

    /* Word 3: "misspell" */
    SpellingBank_LoadWordTriplet_1018_b78c(&session, 3);
    assert(SpellingBank_CheckTypedWord_1018_b35c(&session, "misspell") == 1);

    /* Word 4: "league" -> completes the 4-word round and triggers 0x2714! */
    SpellingBank_LoadWordTriplet_1018_b78c(&session, 4);
    assert(SpellingBank_CheckTypedWord_1018_b35c(&session, "league") == 1);
    assert(session.round_completed_89 == 1);

    /* Verify high score and level-10 badge were updated by 0x2714 (1018:267e) */
    assert(CPlayerRecord_GetHighScore_1018_aeaa(g_DAT_1060_01e4_currentPlayer) == 10);
    assert(CPlayerRecord_GetBadgeFlag_1018_aec0(g_DAT_1060_01e4_currentPlayer, 0) == 1);
    assert(g_DAT_1060_0a46_highScoreTable->nested_250->count_38 == 1);
    assert(g_DAT_1060_0a46_highScoreTable->nested_250->slots_00[0].score_06 == 10);

    /* Test CHEAT sequence (1018:509c) */
    assert(CMainMenuView_HandleKeyAndCheat_1018_509c(cheat_buf, 'c') == 0);
    assert(CMainMenuView_HandleKeyAndCheat_1018_509c(cheat_buf, 'h') == 0);
    assert(CMainMenuView_HandleKeyAndCheat_1018_509c(cheat_buf, 'e') == 0);
    assert(CMainMenuView_HandleKeyAndCheat_1018_509c(cheat_buf, 'a') == 0);
    assert(CMainMenuView_HandleKeyAndCheat_1018_509c(cheat_buf, 't') == 1);
    assert(CPlayerRecord_GetHighScore_1018_aeaa(g_DAT_1060_01e4_currentPlayer) == 100);
}

static void test_spelling_match_river_grid_and_board_scene(void) {
    SBSpellingMatch *match;
    SBSdObRecord sdob_a;
    SBSdObRecord sdob_z;
    SBBoardScene *scene;
    uint16_t frtr_ids[3] = {1000, 1002, 1003};

    /* Verify SdOb #1002 = 'A' (1) and #1027 = 'Z' (26) in SB_CDRES.DLL */
    assert(CActorBehavior_LoadSdObRecord_1018_d0fc(1002, &sdob_a) == 1);
    assert(sdob_a.width_subcells_00 == 4 && sdob_a.height_subcells_02 == 4);
    assert(sdob_a.letter_code_11 == 1);
    assert(CActorBehavior_LoadSdObRecord_1018_d0fc(1027, &sdob_z) == 1);
    assert(sdob_z.letter_code_11 == 26);

    /* Construct CSpellingMatch_1010_cc76 and load real Level 1 (GmTr #1001, GmLt #1001 & #2001) */
    match = CSpellingMatch_Ctor(NULL);
    assert(match != NULL);
    assert(match->grid_10 != NULL);
    assert(match->grid_10->pixel_width_04 == 480);
    assert(match->grid_10->pixel_height_06 == 384);
    assert(match->grid_10->cols_0c == 60);
    assert(match->grid_10->rows_0e == 48);

    assert(CSpellingMatch_LoadLevel_1020_0e50(match, 1) == 1);
    assert(match->grid_10->terrain_sdob_grid[0][0] == 1001);
    assert(CCollection_GetCount_1008_b8bc(match->static_objs_14) == 163);
    assert(CCollection_GetCount_1008_b8bc(match->moving_actors_18) > 0);
    assert(match->player_actor_1c != NULL);

    /* Spell "ache" on the river grid by collecting letters (1020:189e, 1020:1a9a) */
    CSpellingMatch_SetTargetWord_1020_189e(match, "ache");
    CSpellingMatch_Tick_1020_1c9a(match);
    assert(match->match_state_68 == 1);

    assert(CSpellingMatch_CollectLetter_1020_1a9a(match, 1) == 1);  /* 'a' */
    assert(CSpellingMatch_CollectLetter_1020_1a9a(match, 26) == 0); /* wrong letter 'z' rejected */
    assert(CSpellingMatch_CollectLetter_1020_1a9a(match, 3) == 1);  /* 'c' */
    assert(CSpellingMatch_CollectLetter_1020_1a9a(match, 8) == 1);  /* 'h' */
    assert(match->match_state_68 == 1);
    assert(CSpellingMatch_CollectLetter_1020_1a9a(match, 5) == 1);  /* 'e' -> word complete! */
    assert(match->match_state_68 == 4);
    CSpellingMatch_Tick_1020_1c9a(match);
    assert(match->match_state_68 == 5);

    /* Load Board Scene with real PICT #129 and FrTr #1000, #1002, #1003 (1010:6ef4) */
    scene = CBoardScene_InitFromFrTr_1010_6ef4(NULL, NULL, 129, frtr_ids, 3);
    assert(scene != NULL);
    assert(scene->frtr_count_60 == 3);
    assert(scene->last_frtr_id_62 == 1003);

    CBoardScene_Dtor_1010_735e(scene);
    free(scene);
    CSpellingMatch_Dtor(match);
}

int main(void) {
    sb_win16_user_shim_reset();
    sb_win16_gdi_shim_reset();
    sb_win16_waveOut_resetStats();
    assert(sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") == 0);
    ResMgr_LoadCdResLibrary_1008_2bbc();
    assert(CResourceManager_CreateGlobal_1008_2bf4_part() != NULL);
    g_DAT_1060_0a1e_timer60Hz = CTimer60Hz_Ctor_1010_446a(
        (SBTimer60Hz *)NewObject_1008_ef2c(sizeof(SBTimer60Hz)));

    test_str_list_and_word_bank();
    test_player_profile_and_spelling_practice_round();
    test_spelling_match_river_grid_and_board_scene();

    if (g_DAT_1060_01e4_currentPlayer) {
        CPlayerRecord_Dtor_1018_aa84(g_DAT_1060_01e4_currentPlayer);
        g_DAT_1060_01e4_currentPlayer = NULL;
    }
    if (g_DAT_1060_0a46_highScoreTable) {
        CPlayerRecord_Dtor_1018_aa84(g_DAT_1060_0a46_highScoreTable);
        g_DAT_1060_0a46_highScoreTable = NULL;
    }
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

    printf("PASS: test_game_logic (STR#, RlQt, CPlayerRecord, SpellingBank, SdGd, GmTr, GmLt, SdOb, FrTr, CSpellingMatch)\n");
    return 0;
}

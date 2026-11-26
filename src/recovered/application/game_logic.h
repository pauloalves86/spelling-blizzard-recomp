/* Decompiled Spelling Blizzard Game Logic, Spelling Word Bank, River Grid & Actor Subsystems (Path 2)
 *
 * Reconstructs the Bucket C Game Logic classes and methods from `original/BLIZZARD.EXE`
 * operating directly on real assets from `original/SB_CDRES.DLL`:
 *
 * 1. String List (`STR#` `0x03ed`) & Pascal String Helpers (`1010:13b6..1504`):
 *    - `Str_CtoPstr_1010_14c2` (`1010:14c2`)
 *    - `Str_PtoCstr_1010_1504` (`1010:1504`)
 *    - `StrList_GetIndString_1010_13b6` (`1010:13b6`)
 *
 * 2. Player Profile & Top-7 High Score Record (`CPlayerRecord_1008_2b04`, `1018:a67e..af08`, `1008:ce88..eb12`):
 *    - `CPlayerRecord_Ctor_1018_a9a0` (`1018:a9a0`)
 *    - `CPlayerRecord_Dtor_1018_aa84` (`1018:aa84`)
 *    - `RecordTable_InsertTop7_1018_a704` (`1018:a704`)
 *    - `CPlayerRecord_PushRecentLevel_1018_ad88` (`1018:ad88`)
 *    - `CPlayerRecord_AdjustWordWeight_1018_adb8` (`1018:adb8`)
 *    - `CPlayerRecord_UpdateHighScore_1018_ae3a` (`1018:ae3a`)
 *    - `CPlayerRecord_SetBadgeFlag_1018_ae7e` (`1018:ae7e`)
 *    - `CPlayerRecord_GetHighScore_1018_aeaa` (`1018:aeaa`)
 *    - `CPlayerRecord_GetBadgeFlag_1018_aec0` (`1018:aec0`)
 *    - `CPlayerRecord_SetTutorialLevel_1018_aede` (`1018:aede`)
 *    - `CPlayerRecord_HasSeenTutorial_1018_af08` (`1018:af08`)
 *    - `CPlayerRecord_SelectNextWordIndex_1018_acb4` (`1018:acb4`)
 *
 * 3. Spelling Word Bank, Magic Spelling Tricks & `RlQt` Speech Roll (`1018:b190..b950`, `1020:62fe..6b30`):
 *    - `RollQuote_PlaySequence_1020_62fe` (`1020:62fe`)
 *    - `SpellingBank_LoadWordTriplet_1018_b78c` (`1018:b78c`)
 *    - `SpellingBank_CheckTypedWord_1018_b35c` (`1018:b35c`)
 *
 * 4. Spelling Blizzard River Grid (`SdGd`), Level Loader (`GmTr`/`GmLt`), Game Objects (`SdOb`), & Match (`CSpellingMatch_1010_cc76`):
 *    - `CDemonGrid_InitFromSdGd_1010_cb02` (`1010:cb02`, `1018:6ebe`)
 *    - `CActorBehavior_InitFromSdOb_1018_0350` (`1018:0350`, `1018:d0fc`)
 *    - `CSpellingMatch_Reset_1020_0d76` (`1020:0d76`)
 *    - `CSpellingMatch_LoadLevel_1020_0e50` (`1020:0e50`)
 *    - `CSpellingMatch_LoadTerrainGmTr_1020_10d8` (`1020:10d8`)
 *    - `CSpellingMatch_LoadStaticObjectsGmLt_1020_1292` (`1020:1292`)
 *    - `CSpellingMatch_LoadActorsGmLt_1020_1500` (`1020:1500`)
 *    - `CSpellingMatch_CountLetterBlocks_1020_1784` (`1020:1784`)
 *    - `CSpellingMatch_SetTargetWord_1020_189e` (`1020:189e`)
 *    - `CSpellingMatch_StepState_1020_18dc` (`1020:18dc`)
 *    - `CSpellingMatch_CollectLetter_1020_1a9a` (`1020:1a9a`)
 *    - `CSpellingMatch_Tick_1020_1c9a` (`1020:1c9a`)
 *    - `CBoardScene_InitFromFrTr_1010_6ef4` (`1010:6ef4`)
 *
 * 5. Game Controller & HUD Commands (`CGameControllerView_1008_97e6` & `CMainMenuOrMatchView_1018_3228`):
 *    - `CGameController_HandleCommand_1008_ac80` (`1008:ac80`)
 *    - `CGameControllerView_HandleEvent_1018_267e` (`1018:267e`)
 *    - `CGameControllerView_CheckLevelTutorial_1018_284c` (`1018:284c`)
 *    - `CMainMenuView_InitMatchSubViews_1018_43ac` (`1018:43ac`)
 *    - `CMainMenuView_HandleKeyAndCheat_1018_509c` (`1018:509c`)
 */
#ifndef SB_RECOVERED_APPLICATION_GAME_LOGIC_H
#define SB_RECOVERED_APPLICATION_GAME_LOGIC_H

#include <stddef.h>
#include <stdint.h>
#include "../framework/ui_framework.h"
#include "object_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. String List (`STR#` `0x03ed`) & Pascal String Helpers (`1010:13b6..1504`)
 * ========================================================================= */
char *Str_CtoPstr_1010_14c2(char *io_buf);
char *Str_PtoCstr_1010_1504(char *io_buf);
int StrList_GetIndString_1010_13b6(uint16_t str_list_res_id, int16_t index_1based, char *out_cstr, size_t max_len);

/* =========================================================================
 * 2. Player Profile & Top-7 High Score Record (`CPlayerRecord_1008_2b04`)
 * ========================================================================= */
typedef struct SBScoreSlot {
    char name_00[6];    /* +0x00..+0x05: player code/initials */
    int16_t score_06;   /* +0x06: score / level reached */
} SBScoreSlot;

typedef struct SBPlayerNestedData {
    SBScoreSlot slots_00[7];        /* +0x000..+0x037: top-7 score table (or slot[0] for player) */
    int16_t count_38;               /* +0x038: active entries in slots_00 (0..7) */
    int16_t word_weights_08[201];   /* word difficulty weights (1..200, default 100) */
    uint8_t badges_19a[10];         /* +0x19a..+0x1a3: 10 level-group completion badges */
    int32_t total_weight_1a4;       /* +0x1a4: sum of all 201 word weights */
    int16_t tutorial_seen_1a8;      /* +0x1a8: highest tutorial threshold seen */
} SBPlayerNestedData;

typedef struct SBPlayerRecord {
    uint32_t vtable_tag_00;         /* +0x000: 0x10082b04 */
    char player_name_04[32];        /* +0x004: player display name */
    SBPlayerNestedData *nested_250; /* +0x250: nested profile & weight table */
    uint8_t is_dirty_254;           /* +0x254: 1 if modified since save */
    int16_t recent_levels_256[5];   /* +0x256..+0x25f: 5-slot ring buffer of recent words/levels */
    int16_t recent_idx_260;         /* +0x260: ring buffer write cursor (0..4) */
} SBPlayerRecord;

SBPlayerRecord *CPlayerRecord_Ctor_1018_a9a0(SBPlayerRecord *self, const char *name, int16_t initial_score);
void CPlayerRecord_Dtor_1018_aa84(SBPlayerRecord *self);
void RecordTable_InsertTop7_1018_a704(SBPlayerRecord *dest_table, const SBPlayerRecord *source_player);
int16_t CPlayerRecord_PushRecentLevel_1018_ad88(SBPlayerRecord *self, int16_t level_or_word_idx);
void CPlayerRecord_AdjustWordWeight_1018_adb8(SBPlayerRecord *self, uint8_t was_correct, int16_t word_index_1based);
void CPlayerRecord_UpdateHighScore_1018_ae3a(SBPlayerRecord *self, int16_t candidate_score);
void CPlayerRecord_SetBadgeFlag_1018_ae7e(SBPlayerRecord *self, int16_t badge_index);
int16_t CPlayerRecord_GetHighScore_1018_aeaa(const SBPlayerRecord *self);
uint8_t CPlayerRecord_GetBadgeFlag_1018_aec0(const SBPlayerRecord *self, int16_t badge_index);
void CPlayerRecord_SetTutorialLevel_1018_aede(SBPlayerRecord *self, int16_t tutorial_level);
int CPlayerRecord_HasSeenTutorial_1018_af08(const SBPlayerRecord *self, int16_t tutorial_level);
int16_t CPlayerRecord_SelectNextWordIndex_1018_acb4(SBPlayerRecord *self);

/* =========================================================================
 * 3. Spelling Word Bank, Magic Spelling Tricks & `RlQt` Speech Roll
 * ========================================================================= */
typedef struct SBRollQuotePair {
    uint16_t tag_fourcc2; /* e.g. 'hr' (0x6872), 'tl' (0x746c), 'wd' (0x7764), 'se' (0x7365), 'bo' (0x626f) */
    uint16_t item_id;     /* 1-based item/sound/anim index */
} SBRollQuotePair;

typedef struct SBSpellingPracticeSession {
    uint32_t vtable_tag_00;       /* +0x00: 0x1018b35c */
    int16_t current_level_10;     /* +0x10: current level (1..100, mirrors DAT_1060_01de) */
    int16_t current_word_idx_12;  /* +0x12: 1-based word index in STR# #201/#301/#401 */
    int16_t typed_len_14;         /* +0x14: current typed character count */
    uint8_t input_ready_4e;       /* +0x4e: 1 when user submits word */
    char typed_word_52[11];       /* +0x52..+0x5c: player's typed word */
    char primary_word_5d[11];     /* +0x5d..+0x67: primary spelling from STR# #201 */
    char alt1_word_68[11];        /* +0x68..+0x72: alternate spelling 1 from STR# #301 */
    char alt2_word_73[11];        /* +0x73..+0x7d: alternate spelling 2 from STR# #401 */
    int16_t words_correct_82;     /* +0x82: words spelled correctly in this round */
    int16_t target_words_84;      /* +0x84: target words for round = min(10, level/10 + 3) */
    uint8_t word_already_tried_88;/* +0x88: 1 if current word was already attempted */
    uint8_t round_completed_89;   /* +0x89: 1 when words_correct_82 == target_words_84 */
    uint16_t last_quote_tag_8a;   /* +0x8a: last RlQt speech tag triggered */
} SBSpellingPracticeSession;

int RollQuote_PlaySequence_1020_62fe(uint16_t rlqt_res_id,
                                     int16_t seq_index_1based,
                                     SBRollQuotePair *out_pairs,
                                     int max_pairs);
void SpellingBank_LoadWordTriplet_1018_b78c(SBSpellingPracticeSession *session, int16_t word_index_1based);
int SpellingBank_CheckTypedWord_1018_b35c(SBSpellingPracticeSession *session, const char *typed_input);

/* =========================================================================
 * 4. Spelling Blizzard River Grid (`SdGd`), Level Loader (`GmTr`/`GmLt`), Game Objects (`SdOb`), & Match (`CSpellingMatch_1010_cc76`)
 * ========================================================================= */

/* Parsed `'SdOb'` (`0x0403`) Game Object / Actor / Letter Block Definition */
typedef struct SBSdObRecord {
    uint16_t res_id;              /* SdOb resource ID (e.g. 1001..2050) */
    int16_t width_subcells_00;    /* +0x00: width in subcells (typically 4) */
    int16_t height_subcells_02;   /* +0x02: height in subcells (typically 4) */
    uint16_t sprite_res_id_04;    /* +0x04: FrTr / PICT sprite resource ID */
    uint8_t is_player_10;         /* +0x10: 1 if player character */
    char letter_code_11;          /* +0x11: 0 = non-letter, 1..26 = 'A'..'Z', '/' = separator */
    int16_t points_12;            /* +0x12: score delta */
    uint8_t is_hazard_13;         /* +0x13: 1 if hazard/water */
    uint8_t is_pushable_14;       /* +0x14: 1 if pushable ice/letter block */
    uint8_t is_solid_18;          /* +0x18: 1 if solid obstacle */
    uint16_t sound_res_id_16;     /* +0x16: associated `'snd '` resource ID */
} SBSdObRecord;

/* Active Board Actor / Object Instance (`CActorBehavior_1010_90be`, `0xbc` bytes in binary) */
typedef struct SBDemonActor {
    uint32_t vtable_tag_00;       /* +0x00: 0x101090be or 0x101092ae */
    SBSdObRecord sdob_04;         /* +0x04: loaded SdOb definition */
    int16_t grid_x_20;            /* +0x20: subcell X position (0..59) */
    int16_t grid_y_22;            /* +0x22: subcell Y position (0..47) */
    uint16_t extra_param_24;      /* +0x24: parameter from GmLt */
    int16_t dir_46;               /* +0x46: facing/movement direction (0..8) */
    int32_t score_84;             /* +0x84: accumulated actor score */
    uint8_t is_active_88;         /* +0x88: 1 if active on board */
    uint8_t is_frozen_89;         /* +0x89: 1 if frozen/stunned */
    uint16_t child_sdob_ids[8];   /* attached child SdOb resource IDs from GmLt */
    int16_t child_count;
} SBDemonActor;

/* Demon Object Grid (`CDemonGridController_1020_52ee`, loaded from `'SdGd'` `0x0405` #128) */
#define SB_GRID_MAX_COLS 60
#define SB_GRID_MAX_ROWS 48
#define SB_GRID_TILE_COLS 15
#define SB_GRID_TILE_ROWS 12

typedef struct SBDemonGrid {
    uint32_t vtable_tag_00;       /* +0x00: 0x10186ebe / 0x102052ee */
    int16_t pixel_width_04;       /* +0x04: 480 (0x01e0) */
    int16_t pixel_height_06;      /* +0x06: 384 (0x0180) */
    int16_t subcell_w_08;         /* +0x08: 8 */
    int16_t subcell_h_0a;         /* +0x0a: 8 */
    int16_t cols_0c;              /* +0x0c: 60 subcells (480 / 8) */
    int16_t rows_0e;              /* +0x0e: 48 subcells (384 / 8) */
    int16_t tile_step_x_10;       /* +0x10: 4 subcells per tile (15 tile columns) */
    int16_t tile_step_y_12;       /* +0x12: 4 subcells per tile (12 tile rows) */
    uint16_t terrain_sdob_grid[SB_GRID_TILE_ROWS][SB_GRID_TILE_COLS]; /* 15x12 GmTr terrain SdOb IDs */
} SBDemonGrid;

/* Spelling Match Controller (`CSpellingMatch_1010_cc76`, `0x6c` bytes in binary) */
typedef struct SBSpellingMatch {
    uint32_t vtable_tag_00;       /* +0x00: 0x1010cc76 */
    SBDemonGrid *grid_10;         /* +0x10: Demon grid controller (`SdGd` #128) */
    SBCollection *static_objs_14; /* +0x14: `CObList` of static `SBDemonActor *` from `GmLt` #(level+1000) */
    SBCollection *moving_actors_18; /* +0x18: `CObList` of moving `SBDemonActor *` from `GmLt` #(level+2000) */
    SBDemonActor *player_actor_1c;/* +0x1c: pointer to player character actor */
    int16_t level_id_26;          /* +0x26 (`param_2 + 0x13`): current level (1..101) */
    int16_t letter_block_count_48;/* +0x48: count of primary letter blocks on board */
    int16_t extra_letter_count_4a;/* +0x4a: count of secondary letter blocks on board */
    int16_t bonus_mult_4c;        /* +0x4c: bonus multiplier */
    int16_t bonus_base_4e;        /* +0x4e: bonus base points */
    char target_word_pstr_50[11]; /* +0x50..+0x5a: target word (Pascal string at [0], C chars at [1..]) */
    char collected_pstr_5b[11];   /* +0x5b..+0x65: collected letters (Pascal string at [0], C chars at [1..]) */
    int16_t match_state_68;       /* +0x68: 0=init, 1=playing, 2=spawning, 3=active, 4=won, 5=complete */
} SBSpellingMatch;

/* Board Scene View (`CBoardScene_1010_be64`, loads `'FrTr'` `0x040c`) */
typedef struct SBBoardScene {
    SBView view_00;               /* +0x00..+0x37: base `CView_1008_1aac` */
    SBSpellingMatch *match_46;    /* +0x46: active `CSpellingMatch_1010_cc76` */
    SBPictSurface *bg_surface_58; /* +0x58: layered background surface */
    SBPictSurface *comp_surface_5c;/* +0x5c: composite board surface */
    uint16_t frtr_count_60;       /* +0x60: loaded `'FrTr'` frame tree count */
    uint16_t last_frtr_id_62;     /* +0x62: last loaded `'FrTr'` resource ID */
    int16_t frame_counter_64;     /* +0x64: scene animation frame counter */
} SBBoardScene;

/* Segment 1060 Game Globals */
extern uint16_t g_DAT_1060_01de_currentLevel;
extern SBPlayerRecord *g_DAT_1060_01e4_currentPlayer;
extern SBPlayerRecord *g_DAT_1060_0a46_highScoreTable;
extern uint8_t g_DAT_1060_0a36_gameStarted;
extern uint8_t g_DAT_1060_0a52_levelCompleteBadgePending;
extern SBDemonActor *g_DAT_1060_0a32_activePlayerActor;

int CActorBehavior_LoadSdObRecord_1018_d0fc(uint16_t sdob_res_id, SBSdObRecord *out_rec);
SBDemonActor *CActorBehavior_InitFromSdOb_1018_0350(SBDemonActor *self,
                                                    int16_t grid_x,
                                                    int16_t grid_y,
                                                    uint16_t extra_param,
                                                    uint16_t sdob_res_id);
SBDemonGrid *CDemonGrid_InitFromSdGd_1010_cb02(SBDemonGrid *self, uint16_t sdgd_res_id);

SBSpellingMatch *CSpellingMatch_Ctor(SBSpellingMatch *self);
void CSpellingMatch_Dtor(SBSpellingMatch *self);
void CSpellingMatch_Reset_1020_0d76(SBSpellingMatch *self);
int CSpellingMatch_LoadTerrainGmTr_1020_10d8(SBSpellingMatch *self, int16_t level_id);
int CSpellingMatch_LoadStaticObjectsGmLt_1020_1292(SBSpellingMatch *self, int16_t level_id);
int CSpellingMatch_LoadActorsGmLt_1020_1500(SBSpellingMatch *self, int16_t level_id);
void CSpellingMatch_CountLetterBlocks_1020_1784(SBSpellingMatch *self);
int CSpellingMatch_LoadLevel_1020_0e50(SBSpellingMatch *self, int16_t level_id);
int32_t CSpellingMatch_CalcBonusScore_1020_1852(const SBSpellingMatch *self);
int16_t CSpellingMatch_CalcHudCenterOffset_1020_1866(const SBSpellingMatch *self, int16_t cell_w, int16_t box_w);
void CSpellingMatch_SetTargetWord_1020_189e(SBSpellingMatch *self, const char *word_cstr);
void CSpellingMatch_StepState_1020_18dc(SBSpellingMatch *self);
int CSpellingMatch_CollectLetter_1020_1a9a(SBSpellingMatch *self, char letter_code_1to26);
void CSpellingMatch_Tick_1020_1c9a(SBSpellingMatch *self);

SBBoardScene *CBoardScene_InitFromFrTr_1010_6ef4(SBBoardScene *self,
                                                 SBView *superview,
                                                 uint16_t bg_pict_id,
                                                 const uint16_t *frtr_res_ids,
                                                 int16_t frtr_count);
void CBoardScene_Dtor_1010_735e(SBBoardScene *self);

/* =========================================================================
 * 5. Game Controller & HUD Commands (`CGameControllerView_1008_97e6` & `CMainMenuOrMatchView_1018_3228`)
 * ========================================================================= */
void CGameController_HandleCommand_1008_ac80(int16_t cmd_id, int16_t param);
void CGameControllerView_HandleEvent_1018_267e(int16_t event_code);
int CGameControllerView_CheckLevelTutorial_1018_284c(uint8_t skip_tutorial);
int CMainMenuView_HandleKeyAndCheat_1018_509c(char *io_cheat_buf, char key_char);

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOVERED_APPLICATION_GAME_LOGIC_H */

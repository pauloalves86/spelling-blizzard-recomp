/* Decompiled Spelling Blizzard Game Logic, Spelling Word Bank, River Grid & Actor Subsystems (Path 2)
 *
 * Reconstructs the Bucket C Game Logic classes and methods from `original/BLIZZARD.EXE`
 * against the Bright Star Framework (`src/recovered/framework/`) and Thin Win16 Shim (`src/shim/`).
 */
#include "game_logic.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Segment 1060 Game Globals */
uint16_t g_DAT_1060_01de_currentLevel = 1;
SBPlayerRecord *g_DAT_1060_01e4_currentPlayer = NULL;
SBPlayerRecord *g_DAT_1060_0a46_highScoreTable = NULL;
uint8_t g_DAT_1060_0a36_gameStarted = 0;
uint8_t g_DAT_1060_0a52_levelCompleteBadgePending = 0;
SBDemonActor *g_DAT_1060_0a32_activePlayerActor = NULL;

static uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t read_le16s(const uint8_t *p) {
    return (int16_t)read_le16(p);
}

/* =========================================================================
 * 1. String List (`STR#` `0x03ed`) & Pascal String Helpers (`1010:13b6..1504`)
 * ========================================================================= */

/* `1010:14c2` — `Str_CtoPstr_1010_14c2`
 * Converts a NUL-terminated C string in-place to a length-prefixed Pascal string.
 */
char *Str_CtoPstr_1010_14c2(char *io_buf) {
    size_t len;
    if (!io_buf) {
        return NULL;
    }
    len = strlen(io_buf);
    if (len > 254) {
        len = 254;
    }
    memmove(io_buf + 1, io_buf, len);
    io_buf[0] = (char)len;
    return io_buf;
}

/* `1010:1504` — `Str_PtoCstr_1010_1504`
 * Converts a length-prefixed Pascal string in-place to a NUL-terminated C string.
 */
char *Str_PtoCstr_1010_1504(char *io_buf) {
    uint8_t len;
    if (!io_buf) {
        return NULL;
    }
    len = (uint8_t)io_buf[0];
    memmove(io_buf, io_buf + 1, len);
    io_buf[len] = '\0';
    return io_buf;
}

/* `1010:13b6` — `StrList_GetIndString_1010_13b6`
 *
 * Loads a `'STR#'` (`0x03ed`) string list resource from `SB_CDRES.DLL`:
 *   +0x00: uint16_t count (little-endian in Win16 SB_CDRES.DLL)
 *   +0x02: `count` consecutive Pascal strings (`uint8_t len`, `char chars[len]`)
 */
int StrList_GetIndString_1010_13b6(uint16_t str_list_res_id,
                                   int16_t index_1based,
                                   char *out_cstr,
                                   size_t max_len) {
    SBResourceHandle *hStr;
    const uint8_t *p;
    uint32_t sz;
    uint16_t count;
    uint32_t offset = 2;
    int16_t cur;
    uint8_t saved_flags;

    if (!out_cstr || max_len == 0) {
        return 0;
    }
    out_cstr[0] = '\0';
    if (index_1based < 1) {
        return 0;
    }

    hStr = ResMgr_GetResource_1010_10f2(0x03edu, "STR#", str_list_res_id);
    if (!hStr) {
        return 0;
    }
    saved_flags = Handle_GetFlags_1010_0096(hStr);
    Handle_Lock_1010_00e8(hStr);
    p = (const uint8_t *)hStr->data_00;
    sz = hStr->size_08;
    if (!p || sz < 2) {
        Handle_SetFlags_1010_00b0(hStr, saved_flags);
        return 0;
    }

    count = read_le16(p);
    if (index_1based > (int16_t)count) {
        Handle_SetFlags_1010_00b0(hStr, saved_flags);
        return 0;
    }

    for (cur = 1; cur < index_1based && offset < sz; ++cur) {
        uint8_t slen = p[offset];
        offset += 1u + (uint32_t)slen;
    }

    if (offset < sz) {
        uint8_t slen = p[offset];
        size_t copy_len = slen;
        if (offset + 1u + copy_len > sz) {
            copy_len = (size_t)(sz - (offset + 1u));
        }
        if (copy_len >= max_len) {
            copy_len = max_len - 1;
        }
        memcpy(out_cstr, p + offset + 1, copy_len);
        out_cstr[copy_len] = '\0';
        Handle_SetFlags_1010_00b0(hStr, saved_flags);
        return 1;
    }

    Handle_SetFlags_1010_00b0(hStr, saved_flags);
    return 0;
}

/* =========================================================================
 * 2. Player Profile & Top-7 High Score Record (`CPlayerRecord_1008_2b04`)
 * ========================================================================= */

/* `1018:a9a0` — `CPlayerRecord_Ctor_1018_a9a0` */
SBPlayerRecord *CPlayerRecord_Ctor_1018_a9a0(SBPlayerRecord *self,
                                             const char *name,
                                             int16_t initial_score) {
    int i;
    if (!self) {
        self = (SBPlayerRecord *)NewObject_1008_ef2c(sizeof(SBPlayerRecord));
    } else {
        memset(self, 0, sizeof(SBPlayerRecord));
    }
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x10082b04u;
    self->nested_250 = (SBPlayerNestedData *)AllocZeroed_1008_eed8(sizeof(SBPlayerNestedData));
    if (name) {
        strncpy(self->player_name_04, name, sizeof(self->player_name_04) - 1);
        if (self->nested_250) {
            strncpy(self->nested_250->slots_00[0].name_00, name, 5);
            self->nested_250->slots_00[0].name_00[5] = '\0';
        }
    }
    if (self->nested_250) {
        self->nested_250->slots_00[0].score_06 = initial_score;
        self->nested_250->count_38 = (initial_score > 0) ? 1 : 0;
        self->nested_250->total_weight_1a4 = 0;
        for (i = 0; i < 201; ++i) {
            self->nested_250->word_weights_08[i] = 100;
            self->nested_250->total_weight_1a4 += 100;
        }
    }
    return self;
}

/* `1018:aa84` — `CPlayerRecord_Dtor_1018_aa84` */
void CPlayerRecord_Dtor_1018_aa84(SBPlayerRecord *self) {
    if (!self) {
        return;
    }
    if (self->nested_250) {
        FreeMem_1008_ef1e(self->nested_250);
        self->nested_250 = NULL;
    }
    CObject_Dtor_1008_ef8e(self);
}

/* `1018:a704` — `RecordTable_InsertTop7_1018_a704`
 * Inserts or updates `source_player`'s score into `dest_table`'s top-7 descending list.
 */
void RecordTable_InsertTop7_1018_a704(SBPlayerRecord *dest_table, const SBPlayerRecord *source_player) {
    SBPlayerNestedData *dst;
    const SBPlayerNestedData *src;
    char src_name[6];
    int16_t src_score;
    int16_t i;
    int16_t rank = 0;

    if (!dest_table || !source_player || !dest_table->nested_250 || !source_player->nested_250) {
        return;
    }
    dst = dest_table->nested_250;
    src = source_player->nested_250;

    memcpy(src_name, src->slots_00[0].name_00, 6);
    src_name[5] = '\0';
    src_score = src->slots_00[0].score_06;

    /* Remove existing entry with matching name */
    for (i = 0; i < dst->count_38; ++i) {
        if (strcmp(dst->slots_00[i].name_00, src_name) == 0) {
            int16_t shift;
            for (shift = (int16_t)(i + 1); shift < dst->count_38; ++shift) {
                dst->slots_00[shift - 1] = dst->slots_00[shift];
            }
            dst->count_38--;
            break;
        }
    }

    /* Compute descending rank */
    for (i = 0; i < dst->count_38; ++i) {
        if (src_score < dst->slots_00[i].score_06) {
            rank++;
        }
    }

    if (rank < 7) {
        for (i = 6; i > rank; --i) {
            dst->slots_00[i] = dst->slots_00[i - 1];
        }
        memcpy(dst->slots_00[rank].name_00, src_name, 6);
        dst->slots_00[rank].score_06 = src_score;
        dst->count_38++;
        if (dst->count_38 > 7) {
            dst->count_38 = 7;
        }
    }
    dest_table->is_dirty_254 = 1;
}

/* `1018:ad88` — `CPlayerRecord_PushRecentLevel_1018_ad88` */
int16_t CPlayerRecord_PushRecentLevel_1018_ad88(SBPlayerRecord *self, int16_t level_or_word_idx) {
    int16_t idx;
    if (!self) {
        return 0;
    }
    idx = self->recent_idx_260;
    self->recent_levels_256[idx % 5] = level_or_word_idx;
    idx++;
    self->recent_idx_260 = (int16_t)(idx % 5);
    return (int16_t)(idx / 5);
}

/* `1018:adb8` — `CPlayerRecord_AdjustWordWeight_1018_adb8`
 * Adjusts word weight by +40 on miss (`was_correct == 0`) or -20 on hit (`was_correct != 0`),
 * clamped to `[1, 200]`, and updates `total_weight_1a4`.
 */
void CPlayerRecord_AdjustWordWeight_1018_adb8(SBPlayerRecord *self,
                                              uint8_t was_correct,
                                              int16_t word_index_1based) {
    SBPlayerNestedData *nested;
    int16_t old_w;
    int16_t new_w;
    if (!self || !self->nested_250 || word_index_1based < 1 || word_index_1based > 201) {
        return;
    }
    nested = self->nested_250;
    old_w = nested->word_weights_08[word_index_1based - 1];
    new_w = (int16_t)(old_w + (was_correct == 0 ? 40 : -20));
    if (new_w < 1) {
        new_w = 1;
    } else if (new_w > 200) {
        new_w = 200;
    }
    nested->word_weights_08[word_index_1based - 1] = new_w;
    nested->total_weight_1a4 += (int32_t)(new_w - old_w);
    self->is_dirty_254 = 1;
}

/* `1018:ae3a` — `CPlayerRecord_UpdateHighScore_1018_ae3a` */
void CPlayerRecord_UpdateHighScore_1018_ae3a(SBPlayerRecord *self, int16_t candidate_score) {
    if (!self || !self->nested_250) {
        return;
    }
    if (self->nested_250->slots_00[0].score_06 < candidate_score) {
        self->nested_250->slots_00[0].score_06 = candidate_score;
    }
    self->is_dirty_254 = 1;
    if (g_DAT_1060_0a46_highScoreTable) {
        RecordTable_InsertTop7_1018_a704(g_DAT_1060_0a46_highScoreTable, self);
    }
}

/* `1018:ae7e` — `CPlayerRecord_SetBadgeFlag_1018_ae7e` */
void CPlayerRecord_SetBadgeFlag_1018_ae7e(SBPlayerRecord *self, int16_t badge_index) {
    if (!self || !self->nested_250 || badge_index < 0 || badge_index >= 10) {
        return;
    }
    self->nested_250->badges_19a[badge_index] = 1;
    self->is_dirty_254 = 1;
}

/* `1018:aeaa` — `CPlayerRecord_GetHighScore_1018_aeaa` */
int16_t CPlayerRecord_GetHighScore_1018_aeaa(const SBPlayerRecord *self) {
    if (!self || !self->nested_250) {
        return 0;
    }
    return self->nested_250->slots_00[0].score_06;
}

/* `1018:aec0` — `CPlayerRecord_GetBadgeFlag_1018_aec0` */
uint8_t CPlayerRecord_GetBadgeFlag_1018_aec0(const SBPlayerRecord *self, int16_t badge_index) {
    if (!self || !self->nested_250 || badge_index < 0 || badge_index >= 10) {
        return 0;
    }
    return self->nested_250->badges_19a[badge_index];
}

/* `1018:aede` — `CPlayerRecord_SetTutorialLevel_1018_aede` */
void CPlayerRecord_SetTutorialLevel_1018_aede(SBPlayerRecord *self, int16_t tutorial_level) {
    if (!self || !self->nested_250) {
        return;
    }
    self->nested_250->tutorial_seen_1a8 = tutorial_level;
    self->is_dirty_254 = 1;
}

/* `1018:af08` — `CPlayerRecord_HasSeenTutorial_1018_af08` */
int CPlayerRecord_HasSeenTutorial_1018_af08(const SBPlayerRecord *self, int16_t tutorial_level) {
    if (!self || !self->nested_250) {
        return 0;
    }
    return (self->nested_250->tutorial_seen_1a8 >= tutorial_level) ? 1 : 0;
}

/* `1018:acb4` — `CPlayerRecord_SelectNextWordIndex_1018_acb4` */
int16_t CPlayerRecord_SelectNextWordIndex_1018_acb4(SBPlayerRecord *self) {
    int16_t best_idx = 1;
    int16_t best_weight = -1;
    int16_t i;
    if (!self || !self->nested_250) {
        return 1;
    }
    for (i = 1; i <= 201; ++i) {
        int16_t w = self->nested_250->word_weights_08[i - 1];
        if (w > best_weight) {
            best_weight = w;
            best_idx = i;
        }
    }
    return best_idx;
}

/* =========================================================================
 * 3. Spelling Word Bank, Magic Spelling Tricks & `RlQt` Speech Roll
 * ========================================================================= */

/* `1020:62fe` — `RollQuote_PlaySequence_1020_62fe`
 *
 * Parses a Bright Star `'RlQt'` (`0x03eb`) speech/animation sequence table:
 *   +0x00: uint16_t num_sequences (e.g. #1001 has 0x0065 = 101, #2001 has 5, #3001 has 102)
 *   +0x02: For each sequence (1-based `seq_index_1based`):
 *          uint16_t pair_count
 *          followed by `pair_count` 4-byte pairs:
 *            uint16_t tag_fourcc2 (e.g. 'hr', 'tl', 'wd', 'se', 'bo')
 *            uint16_t item_id
 */
int RollQuote_PlaySequence_1020_62fe(uint16_t rlqt_res_id,
                                     int16_t seq_index_1based,
                                     SBRollQuotePair *out_pairs,
                                     int max_pairs) {
    SBResourceHandle *hRlQt;
    const uint8_t *p;
    uint32_t sz;
    uint16_t seq_count;
    uint32_t offset = 2;
    int16_t cur;
    uint16_t pair_count = 0;
    int i;
    uint8_t saved_flags;

    if (seq_index_1based < 1) {
        return 0;
    }
    hRlQt = ResMgr_GetResource_1010_10f2(0x03ebu, "RlQt", rlqt_res_id);
    if (!hRlQt) {
        return 0;
    }
    saved_flags = Handle_GetFlags_1010_0096(hRlQt);
    Handle_Lock_1010_00e8(hRlQt);
    p = (const uint8_t *)hRlQt->data_00;
    sz = hRlQt->size_08;
    if (!p || sz < 4) {
        Handle_SetFlags_1010_00b0(hRlQt, saved_flags);
        return 0;
    }

    seq_count = read_le16(p);
    if (seq_index_1based > (int16_t)seq_count) {
        Handle_SetFlags_1010_00b0(hRlQt, saved_flags);
        return 0;
    }

    for (cur = 1; cur < seq_index_1based && offset + 2 <= sz; ++cur) {
        uint16_t n = read_le16(p + offset);
        offset += 2u + (uint32_t)n * 4u;
    }

    if (offset + 2 <= sz) {
        pair_count = read_le16(p + offset);
        offset += 2;
        for (i = 0; i < (int)pair_count && offset + 4 <= sz; ++i) {
            if (out_pairs && i < max_pairs) {
                out_pairs[i].tag_fourcc2 = read_le16(p + offset);
                out_pairs[i].item_id = read_le16(p + offset + 2);
            }
            offset += 4;
        }
    }

    Handle_SetFlags_1010_00b0(hRlQt, saved_flags);
    return (int)pair_count;
}

/* `1018:b78c` — `SpellingBank_LoadWordTriplet_1018_b78c`
 *
 * Loads the primary spelling word from `'STR#'` `#201` (`+0x5d`),
 * alternate spelling 1 from `'STR#'` `#301` (`+0x68`), and
 * alternate spelling 2 from `'STR#'` `#401` (`+0x73`).
 */
void SpellingBank_LoadWordTriplet_1018_b78c(SBSpellingPracticeSession *session, int16_t word_index_1based) {
    if (!session) {
        return;
    }
    if (word_index_1based < 1) {
        word_index_1based = 1;
    }
    session->current_word_idx_12 = word_index_1based;
    if (!session->word_already_tried_88) {
        session->word_already_tried_88 = 1;
        if (g_DAT_1060_01e4_currentPlayer) {
            CPlayerRecord_PushRecentLevel_1018_ad88(
                g_DAT_1060_01e4_currentPlayer, (int16_t)(word_index_1based - 1));
        }
    }
    StrList_GetIndString_1010_13b6(201, word_index_1based, session->primary_word_5d, sizeof(session->primary_word_5d));
    StrList_GetIndString_1010_13b6(301, word_index_1based, session->alt1_word_68, sizeof(session->alt1_word_68));
    StrList_GetIndString_1010_13b6(401, word_index_1based, session->alt2_word_73, sizeof(session->alt2_word_73));
}

/* `1018:b35c` — `SpellingBank_CheckTypedWord_1018_b35c` */
int SpellingBank_CheckTypedWord_1018_b35c(SBSpellingPracticeSession *session, const char *typed_input) {
    int16_t target_count;
    size_t i;
    int matched = 0;

    if (!session || !typed_input) {
        return 0;
    }
    if (session->current_level_10 <= 0) {
        session->current_level_10 = (int16_t)g_DAT_1060_01de_currentLevel;
    }
    target_count = (int16_t)(session->current_level_10 / 10 + 3);
    if (target_count > 10) {
        target_count = 10;
    }
    session->target_words_84 = target_count;

    /* Copy and normalize uppercase 'A'..'Z' to lowercase 'a'..'z' */
    for (i = 0; i < sizeof(session->typed_word_52) - 1 && typed_input[i] != '\0'; ++i) {
        char c = typed_input[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c + ' ');
        }
        session->typed_word_52[i] = c;
    }
    session->typed_word_52[i] = '\0';
    session->typed_len_14 = (int16_t)i;

    if (strcmp(session->typed_word_52, session->primary_word_5d) == 0) {
        matched = 1;
    } else if (session->alt1_word_68[0] != '\0' &&
               strcmp(session->typed_word_52, session->alt1_word_68) == 0) {
        matched = 1;
    } else if (session->alt2_word_73[0] != '\0' &&
               strcmp(session->typed_word_52, session->alt2_word_73) == 0) {
        matched = 1;
    }

    if (!matched) {
        /* Misspelled: increase word difficulty weight (+40) and play 'rs' (0x7273) quote */
        session->last_quote_tag_8a = 0x7273u;
        if (g_DAT_1060_01e4_currentPlayer) {
            CPlayerRecord_AdjustWordWeight_1018_adb8(
                g_DAT_1060_01e4_currentPlayer, 0, session->current_word_idx_12);
        }
        return 0;
    }

    /* Correct spelling: decrease word difficulty weight (-20), play 'pr' (0x7072), and advance round */
    session->last_quote_tag_8a = 0x7072u;
    if (g_DAT_1060_01e4_currentPlayer) {
        CPlayerRecord_AdjustWordWeight_1018_adb8(
            g_DAT_1060_01e4_currentPlayer, 1, session->current_word_idx_12);
    }
    session->words_correct_82++;
    if (session->words_correct_82 >= target_count) {
        session->round_completed_89 = 1;
        CGameControllerView_HandleEvent_1018_267e(0x2714);
    }
    return 1;
}

/* =========================================================================
 * 4. Spelling Blizzard River Grid (`SdGd`), Level Loader (`GmTr`/`GmLt`),
 *    Game Objects (`SdOb`), & Match (`CSpellingMatch_1010_cc76`)
 * ========================================================================= */

/* `1018:d0fc` — `CActorBehavior_LoadSdObRecord_1018_d0fc`
 *
 * Parses a 256-byte `'SdOb'` (`0x0403`) resource from `SB_CDRES.DLL`:
 *   +0x00: int16_t  width_subcells (typically 4)
 *   +0x02: int16_t  height_subcells (typically 4)
 *   +0x04: uint16_t sprite_res_id (e.g. 0x05dc = 1500)
 *   +0x10: uint8_t  is_player
 *   +0x11: char     letter_code (1..26 for 'A'..'Z', '/' for ice separator, 0 for terrain/demon)
 *   +0x12: int16_t  points
 *   +0x13: uint8_t  is_hazard
 *   +0x14: uint8_t  is_pushable
 *   +0x16: uint16_t sound_res_id
 *   +0x18: uint8_t  is_solid
 */
int CActorBehavior_LoadSdObRecord_1018_d0fc(uint16_t sdob_res_id, SBSdObRecord *out_rec) {
    SBResourceHandle *hSdOb;
    const uint8_t *p;
    uint32_t sz;
    uint8_t saved_flags;

    if (!out_rec) {
        return 0;
    }
    memset(out_rec, 0, sizeof(SBSdObRecord));
    out_rec->res_id = sdob_res_id;

    hSdOb = ResMgr_GetResource_1010_10f2(0x0403u, "SdOb", sdob_res_id);
    if (!hSdOb) {
        return 0;
    }
    saved_flags = Handle_GetFlags_1010_0096(hSdOb);
    Handle_Lock_1010_00e8(hSdOb);
    p = (const uint8_t *)hSdOb->data_00;
    sz = hSdOb->size_08;
    if (!p || sz < 0x20) {
        Handle_SetFlags_1010_00b0(hSdOb, saved_flags);
        return 0;
    }

    out_rec->width_subcells_00 = read_le16s(p + 0x00);
    out_rec->height_subcells_02 = read_le16s(p + 0x02);
    out_rec->sprite_res_id_04 = read_le16(p + 0x04);
    out_rec->is_player_10 = p[0x10];
    out_rec->letter_code_11 = (char)p[0x12];
    out_rec->points_12 = read_le16s(p + 0x14);
    out_rec->is_hazard_13 = p[0x13];
    out_rec->is_pushable_14 = p[0x14];
    out_rec->sound_res_id_16 = read_le16(p + 0x16);
    out_rec->is_solid_18 = p[0x18];

    Handle_SetFlags_1010_00b0(hSdOb, saved_flags);
    return 1;
}

/* `1018:0350` — `CActorBehavior_InitFromSdOb_1018_0350` */
SBDemonActor *CActorBehavior_InitFromSdOb_1018_0350(SBDemonActor *self,
                                                    int16_t grid_x,
                                                    int16_t grid_y,
                                                    uint16_t extra_param,
                                                    uint16_t sdob_res_id) {
    if (!self) {
        self = (SBDemonActor *)NewObject_1008_ef2c(sizeof(SBDemonActor));
    } else {
        memset(self, 0, sizeof(SBDemonActor));
    }
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x101090beu;
    CActorBehavior_LoadSdObRecord_1018_d0fc(sdob_res_id, &self->sdob_04);
    self->grid_x_20 = grid_x;
    self->grid_y_22 = grid_y;
    self->extra_param_24 = extra_param;
    self->is_active_88 = 1;
    return self;
}

/* `1010:cb02` & `1018:6ebe` — `CDemonGrid_InitFromSdGd_1010_cb02`
 *
 * Parses `'SdGd'` (`0x0405` #128) from `SB_CDRES.DLL`:
 *   +0x00: int16_t pixel_width  = 0x01e0 (480)
 *   +0x02: int16_t pixel_height = 0x0180 (384)
 *   +0x04: int16_t subcell_w    = 8
 *   +0x06: int16_t subcell_h    = 8
 *   +0x08: int16_t tile_step_x  = 4
 *   +0x0a: int16_t tile_step_y  = 4
 */
SBDemonGrid *CDemonGrid_InitFromSdGd_1010_cb02(SBDemonGrid *self, uint16_t sdgd_res_id) {
    SBResourceHandle *hSdGd;
    const uint8_t *p;
    uint8_t saved_flags;

    if (!self) {
        self = (SBDemonGrid *)NewObject_1008_ef2c(sizeof(SBDemonGrid));
    } else {
        memset(self, 0, sizeof(SBDemonGrid));
    }
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x102052eeu;
    self->pixel_width_04 = 480;
    self->pixel_height_06 = 384;
    self->subcell_w_08 = 8;
    self->subcell_h_0a = 8;
    self->tile_step_x_10 = 4;
    self->tile_step_y_12 = 4;

    hSdGd = ResMgr_GetResource_1010_10f2(0x0405u, "SdGd", sdgd_res_id);
    if (hSdGd) {
        saved_flags = Handle_GetFlags_1010_0096(hSdGd);
        Handle_Lock_1010_00e8(hSdGd);
        p = (const uint8_t *)hSdGd->data_00;
        if (p && hSdGd->size_08 >= 14) {
            self->pixel_width_04 = read_le16s(p + 0);
            self->pixel_height_06 = read_le16s(p + 2);
            self->subcell_w_08 = read_le16s(p + 4);
            self->subcell_h_0a = read_le16s(p + 6);
            self->tile_step_x_10 = read_le16s(p + 8);
            self->tile_step_y_12 = read_le16s(p + 10);
        }
        Handle_SetFlags_1010_00b0(hSdGd, saved_flags);
    }
    self->cols_0c = (self->subcell_w_08 > 0)
                        ? (int16_t)(self->pixel_width_04 / self->subcell_w_08)
                        : 60;
    self->rows_0e = (self->subcell_h_0a > 0)
                        ? (int16_t)(self->pixel_height_06 / self->subcell_h_0a)
                        : 48;
    return self;
}

SBSpellingMatch *CSpellingMatch_Ctor(SBSpellingMatch *self) {
    if (!self) {
        self = (SBSpellingMatch *)NewObject_1008_ef2c(sizeof(SBSpellingMatch));
    } else {
        memset(self, 0, sizeof(SBSpellingMatch));
    }
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x1010cc76u;
    self->grid_10 = CDemonGrid_InitFromSdGd_1010_cb02(NULL, 128);
    self->static_objs_14 = CObList_New_1010_036c();
    self->moving_actors_18 = CObList_New_1010_036c();
    self->bonus_mult_4c = 5;
    self->bonus_base_4e = 100;
    return self;
}

void CSpellingMatch_Dtor(SBSpellingMatch *self) {
    if (!self) {
        return;
    }
    if (self->static_objs_14) {
        CObList_DisposeAndFree_1008_c668(self->static_objs_14);
        CObList_Dtor_1008_c11a(self->static_objs_14);
        free(self->static_objs_14);
        self->static_objs_14 = NULL;
    }
    if (self->moving_actors_18) {
        CObList_DisposeAndFree_1008_c668(self->moving_actors_18);
        CObList_Dtor_1008_c11a(self->moving_actors_18);
        free(self->moving_actors_18);
        self->moving_actors_18 = NULL;
    }
    if (self->grid_10) {
        FreeMem_1008_ef1e(self->grid_10);
        self->grid_10 = NULL;
    }
    CObject_Dtor_1008_ef8e(self);
}

/* `1020:0d76` — `CSpellingMatch_Reset_1020_0d76` */
void CSpellingMatch_Reset_1020_0d76(SBSpellingMatch *self) {
    if (!self) {
        return;
    }
    self->match_state_68 = 0;
    self->collected_pstr_5b[0] = 0;
    self->collected_pstr_5b[1] = '\0';
    self->player_actor_1c = NULL;
    g_DAT_1060_0a32_activePlayerActor = NULL;
    g_DAT_1060_0a52_levelCompleteBadgePending = 0;
    if (self->static_objs_14) {
        CObList_DisposeAndFree_1008_c668(self->static_objs_14);
    }
    if (self->moving_actors_18) {
        CObList_DisposeAndFree_1008_c668(self->moving_actors_18);
    }
}

/* `1020:10d8` — `CSpellingMatch_LoadTerrainGmTr_1020_10d8`
 *
 * Loads `'GmTr'` (`0x0406`) `#(level_id + 1000)` and populates the 15x12 terrain tile grid.
 */
int CSpellingMatch_LoadTerrainGmTr_1020_10d8(SBSpellingMatch *self, int16_t level_id) {
    SBResourceHandle *hGmTr;
    const uint8_t *p;
    uint32_t sz;
    uint8_t saved_flags;
    int16_t y;
    int16_t x;
    uint32_t offset = 0;
    int tiles_loaded = 0;

    if (!self || !self->grid_10) {
        return 0;
    }
    hGmTr = ResMgr_GetResource_1010_10f2(0x0406u, "GmTr", (uint16_t)(level_id + 1000));
    if (!hGmTr) {
        return 0;
    }
    saved_flags = Handle_GetFlags_1010_0096(hGmTr);
    if (g_DAT_1060_07f5_preloadFlag) {
        Handle_NoPurge_1010_015c(hGmTr);
        Handle_Unlock_1010_0122(hGmTr);
        Handle_MoveNeck_1010_0608(hGmTr);
    }
    Handle_Lock_1010_00e8(hGmTr);
    p = (const uint8_t *)hGmTr->data_00;
    sz = hGmTr->size_08;

    for (y = 0; y < self->grid_10->rows_0e; y = (int16_t)(y + 4)) {
        for (x = 0; x < self->grid_10->cols_0c; x = (int16_t)(x + 4)) {
            if (p && offset + 2 <= sz) {
                uint16_t sdob_id = read_le16(p + offset);
                int tr = y / 4;
                int tc = x / 4;
                if (tr < SB_GRID_TILE_ROWS && tc < SB_GRID_TILE_COLS) {
                    self->grid_10->terrain_sdob_grid[tr][tc] = sdob_id;
                }
                offset += 2;
                tiles_loaded++;
            }
        }
    }

    Handle_SetFlags_1010_00b0(hGmTr, saved_flags);
    return tiles_loaded;
}

/* Shared parser for `GmLt` (`0x0407`) used by `1020:1292` (#(level+1000)) and `1020:1500` (#(level+2000)) */
static int load_gmlt_objects(SBSpellingMatch *self,
                             uint16_t gmlt_res_id,
                             SBCollection *target_list,
                             int is_actor_layer) {
    SBResourceHandle *hGmLt;
    const uint8_t *p;
    uint32_t sz;
    uint8_t saved_flags;
    int16_t count;
    int16_t i;
    uint32_t offset = 2;
    int loaded = 0;

    if (!self || !target_list) {
        return 0;
    }
    hGmLt = ResMgr_GetResource_1010_10f2(0x0407u, "GmLt", gmlt_res_id);
    if (!hGmLt) {
        return 0;
    }
    saved_flags = Handle_GetFlags_1010_0096(hGmLt);
    if (g_DAT_1060_07f5_preloadFlag) {
        Handle_NoPurge_1010_015c(hGmLt);
        Handle_Unlock_1010_0122(hGmLt);
        Handle_MoveNeck_1010_0608(hGmLt);
    }
    Handle_Lock_1010_00e8(hGmLt);
    p = (const uint8_t *)hGmLt->data_00;
    sz = hGmLt->size_08;
    if (!p || sz < 2) {
        Handle_SetFlags_1010_00b0(hGmLt, saved_flags);
        return 0;
    }

    count = read_le16s(p);
    for (i = 0; i < count && offset + 10 <= sz; ++i) {
        uint16_t sdob_id = read_le16(p + offset + 0);
        int16_t gx = read_le16s(p + offset + 2);
        int16_t gy = read_le16s(p + offset + 4);
        uint16_t extra = read_le16(p + offset + 6);
        int16_t sub_count = read_le16s(p + offset + 8);
        SBDemonActor *actor;
        int16_t c;

        offset += 10;
        actor = CActorBehavior_InitFromSdOb_1018_0350(NULL, gx, gy, extra, sdob_id);
        if (actor) {
            actor->child_count = 0;
            for (c = 0; c < sub_count && offset + 2 <= sz; ++c) {
                uint16_t child_sdob = read_le16(p + offset);
                offset += 2;
                if (c < 8) {
                    actor->child_sdob_ids[actor->child_count++] = child_sdob;
                }
            }
            CObList_InsertLast_1008_c520(target_list, actor);
            if (is_actor_layer && (actor->sdob_04.is_player_10 || !self->player_actor_1c)) {
                self->player_actor_1c = actor;
                g_DAT_1060_0a32_activePlayerActor = actor;
            }
            loaded++;
        } else {
            offset += (uint32_t)sub_count * 2u;
        }
    }

    Handle_SetFlags_1010_00b0(hGmLt, saved_flags);
    return loaded;
}

/* `1020:1292` — `CSpellingMatch_LoadStaticObjectsGmLt_1020_1292` */
int CSpellingMatch_LoadStaticObjectsGmLt_1020_1292(SBSpellingMatch *self, int16_t level_id) {
    return load_gmlt_objects(self, (uint16_t)(level_id + 1000), self->static_objs_14, 0);
}

/* `1020:1500` — `CSpellingMatch_LoadActorsGmLt_1020_1500` */
int CSpellingMatch_LoadActorsGmLt_1020_1500(SBSpellingMatch *self, int16_t level_id) {
    return load_gmlt_objects(self, (uint16_t)(level_id + 2000), self->moving_actors_18, 1);
}

/* `1020:1784` — `CSpellingMatch_CountLetterBlocks_1020_1784` */
void CSpellingMatch_CountLetterBlocks_1020_1784(SBSpellingMatch *self) {
    int16_t i;
    int16_t primary = 0;
    int16_t extra = 0;
    if (!self || !self->static_objs_14) {
        return;
    }
    for (i = 1; i <= self->static_objs_14->count_04; ++i) {
        SBDemonActor *obj = (SBDemonActor *)CObList_At_1008_c3ba(self->static_objs_14, i);
        if (obj && obj->sdob_04.letter_code_11 != 0) {
            if (obj->sdob_04.letter_code_11 == '/') {
                extra++;
            } else {
                primary++;
            }
        }
    }
    self->letter_block_count_48 = primary;
    self->extra_letter_count_4a = extra;
}

/* `1020:0e50` — `CSpellingMatch_LoadLevel_1020_0e50` */
int CSpellingMatch_LoadLevel_1020_0e50(SBSpellingMatch *self, int16_t level_id) {
    int tiles;
    int statics;
    int actors;
    if (!self) {
        return 0;
    }
    CSpellingMatch_Reset_1020_0d76(self);
    self->level_id_26 = level_id;
    self->match_state_68 = 0;
    tiles = CSpellingMatch_LoadTerrainGmTr_1020_10d8(self, level_id);
    statics = CSpellingMatch_LoadStaticObjectsGmLt_1020_1292(self, level_id);
    actors = CSpellingMatch_LoadActorsGmLt_1020_1500(self, level_id);
    CSpellingMatch_CountLetterBlocks_1020_1784(self);
    g_DAT_1060_0a52_levelCompleteBadgePending = 0;
    return (tiles > 0 && statics > 0 && actors > 0) ? 1 : 0;
}

/* `1020:1852` — `CSpellingMatch_CalcBonusScore_1020_1852` */
int32_t CSpellingMatch_CalcBonusScore_1020_1852(const SBSpellingMatch *self) {
    if (!self) {
        return 0;
    }
    return (int32_t)(self->bonus_mult_4c - 1) * (int32_t)self->bonus_base_4e;
}

/* `1020:1866` — `CSpellingMatch_CalcHudCenterOffset_1020_1866` */
int16_t CSpellingMatch_CalcHudCenterOffset_1020_1866(const SBSpellingMatch *self,
                                                     int16_t cell_w,
                                                     int16_t box_w) {
    if (!self) {
        return 0;
    }
    return (int16_t)(((self->letter_block_count_48 + 4) * cell_w - box_w) / 2);
}

/* `1020:189e` — `CSpellingMatch_SetTargetWord_1020_189e` */
void CSpellingMatch_SetTargetWord_1020_189e(SBSpellingMatch *self, const char *word_cstr) {
    if (!self || !word_cstr) {
        return;
    }
    memset(self->target_word_pstr_50, 0, sizeof(self->target_word_pstr_50));
    strncpy(self->target_word_pstr_50, word_cstr, sizeof(self->target_word_pstr_50) - 1);
    memset(self->collected_pstr_5b, 0, sizeof(self->collected_pstr_5b));
}

/* `1020:18dc` — `CSpellingMatch_StepState_1020_18dc` */
void CSpellingMatch_StepState_1020_18dc(SBSpellingMatch *self) {
    if (!self) {
        return;
    }
    if (self->match_state_68 == 2) {
        g_DAT_1060_0a32_activePlayerActor = NULL;
        self->match_state_68 = 3;
    }
}

/* `1020:1a9a` — `CSpellingMatch_CollectLetter_1020_1a9a`
 *
 * Called when the player pushes/collects a letter block on the river grid.
 * `letter_code_1to26` is either 1..26 (from `SdOb+0x11`), or ASCII `'A'..'Z'` / `'a'..'z'`.
 */
int CSpellingMatch_CollectLetter_1020_1a9a(SBSpellingMatch *self, char letter_code_1to26) {
    char ch;
    size_t cur_len;
    size_t target_len;
    size_t i;
    int mismatch = 0;

    if (!self) {
        return 0;
    }
    if (letter_code_1to26 >= 1 && letter_code_1to26 <= 26) {
        ch = (char)('a' + (letter_code_1to26 - 1));
    } else if (letter_code_1to26 >= 'A' && letter_code_1to26 <= 'Z') {
        ch = (char)(letter_code_1to26 + ' ');
    } else {
        ch = letter_code_1to26;
    }

    cur_len = strlen(self->collected_pstr_5b);
    if (cur_len + 1 < sizeof(self->collected_pstr_5b)) {
        self->collected_pstr_5b[cur_len] = ch;
        self->collected_pstr_5b[cur_len + 1] = '\0';
        cur_len++;
    }

    /* Convert both to Pascal strings temporarily as in 1020:1a9a (`1010:14c2`) */
    Str_CtoPstr_1010_14c2(self->collected_pstr_5b);
    Str_CtoPstr_1010_14c2(self->target_word_pstr_50);

    cur_len = (uint8_t)self->collected_pstr_5b[0];
    target_len = (uint8_t)self->target_word_pstr_50[0];

    for (i = 1; i <= cur_len; ++i) {
        char tc = self->target_word_pstr_50[i];
        char cc = self->collected_pstr_5b[i];
        if (tc >= 'A' && tc <= 'Z') {
            tc = (char)(tc + ' ');
        }
        if (cc >= 'A' && cc <= 'Z') {
            cc = (char)(cc + ' ');
        }
        if (tc != cc) {
            mismatch = 1;
            break;
        }
    }

    if (!mismatch) {
        if (cur_len == target_len && target_len > 0) {
            self->match_state_68 = 4; /* Level word completed! */
            if (g_DAT_1060_01e4_currentPlayer) {
                CPlayerRecord_AdjustWordWeight_1018_adb8(
                    g_DAT_1060_01e4_currentPlayer, 1, (int16_t)g_DAT_1060_01de_currentLevel);
            }
        }
    } else {
        /* Wrong letter collected: remove the mismatched letter and apply penalty (+40) */
        self->collected_pstr_5b[0] = (char)(cur_len - 1);
        if (g_DAT_1060_01e4_currentPlayer) {
            CPlayerRecord_AdjustWordWeight_1018_adb8(
                g_DAT_1060_01e4_currentPlayer, 0, (int16_t)g_DAT_1060_01de_currentLevel);
        }
    }

    Str_PtoCstr_1010_1504(self->collected_pstr_5b);
    Str_PtoCstr_1010_1504(self->target_word_pstr_50);
    return mismatch ? 0 : 1;
}

/* `1020:1c9a` — `CSpellingMatch_Tick_1020_1c9a` */
void CSpellingMatch_Tick_1020_1c9a(SBSpellingMatch *self) {
    if (!self) {
        return;
    }
    if (self->match_state_68 == 0) {
        self->match_state_68 = 1;
    } else if (self->match_state_68 == 4) {
        self->match_state_68 = 5;
    }
}

/* `1010:6ef4` — `CBoardScene_InitFromFrTr_1010_6ef4` */
SBBoardScene *CBoardScene_InitFromFrTr_1010_6ef4(SBBoardScene *self,
                                                 SBView *superview,
                                                 uint16_t bg_pict_id,
                                                 const uint16_t *frtr_res_ids,
                                                 int16_t frtr_count) {
    SB_RECT16 board_rc = {0, 0, 640, 480};
    int16_t i;

    if (!self) {
        self = (SBBoardScene *)NewObject_1008_ef2c(sizeof(SBBoardScene));
    } else {
        memset(self, 0, sizeof(SBBoardScene));
    }
    if (!self) {
        return NULL;
    }
    CView_Ctor_1010_15f0(&self->view_00, superview, NULL, 0, 0, 640, 480, 0);
    self->view_00.base_00.vtable_tag_00 = 0x1010be64u;

    self->bg_surface_58 = (SBPictSurface *)NewObject_1008_ef2c(sizeof(SBPictSurface));
    if (self->bg_surface_58) {
        CLayeredOffscreen_InitFromPict_1008_5e14(self->bg_surface_58, 1, 8, bg_pict_id);
    }

    self->comp_surface_5c = (SBPictSurface *)NewObject_1008_ef2c(sizeof(SBPictSurface));
    if (self->comp_surface_5c) {
        CPictSurface_InitBlank_1008_5646(self->comp_surface_5c, 1, 8, &board_rc);
    }

    self->frtr_count_60 = 0;
    for (i = 0; i < frtr_count && frtr_res_ids; ++i) {
        SBResourceHandle *hFrTr = ResMgr_GetResource_1010_10f2(0x040cu, "FrTr", frtr_res_ids[i]);
        if (hFrTr) {
            self->frtr_count_60++;
            self->last_frtr_id_62 = frtr_res_ids[i];
        }
    }
    self->frame_counter_64 = 1;
    return self;
}

/* `1010:735e` — `CBoardScene_Dtor_1010_735e` */
void CBoardScene_Dtor_1010_735e(SBBoardScene *self) {
    if (!self) {
        return;
    }
    if (self->bg_surface_58) {
        CPictSurface_Dtor_1008_56a8(self->bg_surface_58);
        self->bg_surface_58 = NULL;
    }
    if (self->comp_surface_5c) {
        CPictSurface_Dtor_1008_56a8(self->comp_surface_5c);
        self->comp_surface_5c = NULL;
    }
    CView_Dtor_1010_16d4(&self->view_00);
}

/* =========================================================================
 * 5. Game Controller & HUD Commands (`CGameControllerView_1008_97e6` & `CMainMenuOrMatchView_1018_3228`)
 * ========================================================================= */

/* `1008:ac80` — `CGameController_HandleCommand_1008_ac80` */
void CGameController_HandleCommand_1008_ac80(int16_t cmd_id, int16_t param) {
    (void)param;
    if (cmd_id == 0x2af9) {
        if (g_DAT_1060_01e4_currentPlayer) {
            CPlayerRecord_Dtor_1018_aa84(g_DAT_1060_01e4_currentPlayer);
            g_DAT_1060_01e4_currentPlayer = NULL;
        }
        g_DAT_1060_01e4_currentPlayer = CPlayerRecord_Ctor_1018_a9a0(NULL, "PLAYR", 1);
        g_DAT_1060_01de_currentLevel = (uint16_t)CPlayerRecord_GetHighScore_1018_aeaa(g_DAT_1060_01e4_currentPlayer);
        if (g_DAT_1060_01de_currentLevel == 0) {
            g_DAT_1060_01de_currentLevel = 1;
        }
        g_DAT_1060_0a36_gameStarted = 0;
    }
}

/* `1018:267e` — `CGameControllerView_HandleEvent_1018_267e` */
void CGameControllerView_HandleEvent_1018_267e(int16_t event_code) {
    if (event_code == 0x2714) {
        if (g_DAT_1060_0a36_gameStarted == 0) {
            g_DAT_1060_0a36_gameStarted = 1;
            return;
        }
        if (g_DAT_1060_01e4_currentPlayer) {
            CPlayerRecord_UpdateHighScore_1018_ae3a(
                g_DAT_1060_01e4_currentPlayer, (int16_t)g_DAT_1060_01de_currentLevel);
            if (g_DAT_1060_0a52_levelCompleteBadgePending != 0 && g_DAT_1060_01de_currentLevel >= 10) {
                CPlayerRecord_SetBadgeFlag_1018_ae7e(
                    g_DAT_1060_01e4_currentPlayer, (int16_t)(g_DAT_1060_01de_currentLevel / 10 - 1));
            }
        }
    }
}

/* `1018:284c` — `CGameControllerView_CheckLevelTutorial_1018_284c`
 * Checks level thresholds (1, 11, 21, 31, 41, 51, 61, 71, 81, 91) and marks the
 * tutorial threshold in the player profile via `1018:af08` and `1018:aede`.
 */
int CGameControllerView_CheckLevelTutorial_1018_284c(uint8_t skip_tutorial) {
    int16_t thresh = 0;
    if (skip_tutorial || !g_DAT_1060_01e4_currentPlayer) {
        return 0;
    }
    if (g_DAT_1060_01de_currentLevel == 1) {
        thresh = 1;
    } else if ((g_DAT_1060_01de_currentLevel % 10) == 1 && g_DAT_1060_01de_currentLevel <= 91) {
        thresh = (int16_t)(g_DAT_1060_01de_currentLevel / 10 + 1);
    }
    if (thresh > 0 && !CPlayerRecord_HasSeenTutorial_1018_af08(g_DAT_1060_01e4_currentPlayer, thresh)) {
        CPlayerRecord_SetTutorialLevel_1018_aede(g_DAT_1060_01e4_currentPlayer, thresh);
        return thresh;
    }
    return 0;
}

/* `1018:509c` — `CMainMenuView_HandleKeyAndCheat_1018_509c`
 * Accumulates key presses in `io_cheat_buf` (up to 5 chars) and detects the `"CHEAT"` sequence.
 */
int CMainMenuView_HandleKeyAndCheat_1018_509c(char *io_cheat_buf, char key_char) {
    size_t len;
    char up;
    if (!io_cheat_buf) {
        return 0;
    }
    up = (char)toupper((unsigned char)key_char);
    len = strlen(io_cheat_buf);
    if (len >= 5) {
        memmove(io_cheat_buf, io_cheat_buf + 1, 4);
        io_cheat_buf[4] = up;
        io_cheat_buf[5] = '\0';
    } else {
        io_cheat_buf[len] = up;
        io_cheat_buf[len + 1] = '\0';
    }
    if (strcmp(io_cheat_buf, "CHEAT") == 0) {
        if (g_DAT_1060_01e4_currentPlayer) {
            CPlayerRecord_UpdateHighScore_1018_ae3a(g_DAT_1060_01e4_currentPlayer, 100);
        }
        return 1;
    }
    return 0;
}

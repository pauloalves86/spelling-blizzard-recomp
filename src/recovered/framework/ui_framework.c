/* Decompiled Bright Star Core Collections, Event Dispatcher, View, Window, Desktop & Application Framework (Path 2)
 *
 * Reconstructs the 193 functions of the core UI and application framework from
 * `original/BLIZZARD.EXE` against the Thin Win16 API Shim (`src/shim/win16_user.h`,
 * `src/shim/win16_gdi.h`, `src/shim/win16_resource.h`, `src/shim/win16_waveout.h`).
 */
#include "ui_framework.h"
#include <stdlib.h>
#include <string.h>

/* Segment 1060 Framework Globals */
SBApplication *g_DAT_1060_07c4_app = NULL;
SBDesktop *g_DAT_1060_0810_desktop = NULL;
SBEventDispatcher *g_DAT_1060_0836_dispatcher = NULL;
SBCollection *g_DAT_1060_0a2e_idleList = NULL;
SBView *g_DAT_1060_0828_focusedView = NULL;
uint16_t g_DAT_1060_1d58_hInstance = 1;
uint16_t g_DAT_1060_1d56_nCmdShow = 1;
uint8_t g_DAT_1060_07f4_appReady = 0;
void *g_DAT_1060_07f6_gameController = NULL;

static uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static int16_t read_be16s(const uint8_t *p) {
    return (int16_t)read_be16(p);
}

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

/* =========================================================================
 * 1. Core Collections & Event Target (`1008:b6f8..c762`, `1010:036c`, `1010:1246`)
 * ========================================================================= */

/* `1008:b6f8` — `CEventTarget_Ctor_1008_b6f8` */
SBEventTarget *CEventTarget_Ctor_1008_b6f8(SBEventTarget *self) {
    if (!self) {
        self = (SBEventTarget *)calloc(1, sizeof(SBEventTarget));
    } else {
        memset(self, 0, sizeof(SBEventTarget));
    }
    if (self) {
        self->vtable_tag_00 = 0x100807f4u;
        self->next_handler_04 = NULL;
    }
    return self;
}

/* `1008:b714` — `CEventTarget_Dtor_1008_b714` */
void CEventTarget_Dtor_1008_b714(SBEventTarget *self) {
    if (!self) {
        return;
    }
    self->vtable_tag_00 = 0x100807f4u;
    self->next_handler_04 = NULL;
}

/* `1008:b732` — `CEventTarget_Dispose_1008_b732` */
void CEventTarget_Dispose_1008_b732(SBEventTarget *self) {
    if (!self) {
        return;
    }
    CEventTarget_Dtor_1008_b714(self);
}

/* `1008:b73a` — `CEventTarget_HandleCommand_1008_b73a` */
int CEventTarget_HandleCommand_1008_b73a(SBEventTarget *self, int32_t cmd) {
    if (self && self->next_handler_04) {
        return CEventTarget_HandleCommand_1008_b73a(self->next_handler_04, cmd);
    }
    return 0;
}

/* `1008:b74e` — `CEventTarget_HandleKey_1008_b74e` */
int CEventTarget_HandleKey_1008_b74e(SBEventTarget *self, uint16_t key, uint16_t modifiers) {
    if (self && self->next_handler_04) {
        return CEventTarget_HandleKey_1008_b74e(self->next_handler_04, key, modifiers);
    }
    return 0;
}

/* `1008:b752` — `CEventTarget_UpdateMenus_1008_b752` */
void CEventTarget_UpdateMenus_1008_b752(SBEventTarget *self) {
    if (self && self->next_handler_04) {
        CEventTarget_UpdateMenus_1008_b752(self->next_handler_04);
    }
}

/* `1008:b758` — `CEventTarget_Idle_1008_b758` */
void CEventTarget_Idle_1008_b758(SBEventTarget *self) {
    (void)self;
}

/* `1008:b75c` — `CEventTarget_GetNextHandler_1008_b75c` */
SBEventTarget *CEventTarget_GetNextHandler_1008_b75c(const SBEventTarget *self) {
    return self ? self->next_handler_04 : NULL;
}

/* `1008:b760` — `CCollection_Ctor_1008_b760` */
SBCollection *CCollection_Ctor_1008_b760(SBCollection *self, int16_t elem_size) {
    if (!self) {
        self = (SBCollection *)calloc(1, sizeof(SBCollection));
    } else {
        memset(self, 0, sizeof(SBCollection));
    }
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x10080824u;
    self->count_04 = 0;
    self->elem_size_06 = (elem_size > 0) ? elem_size : (int16_t)sizeof(void *);
    self->chunk_grow_08 = (int16_t)(self->elem_size_06 * 8);
    self->alloc_bytes_0a = 0;
    self->lock_depth_0c = 0;
    self->data_0e = NULL;
    self->is_sorted_12 = 0;
    return self;
}

/* `1008:b7b8` — `CCollection_Dtor_1008_b7b8` */
void CCollection_Dtor_1008_b7b8(SBCollection *self) {
    if (!self) {
        return;
    }
    if (self->data_0e) {
        free(self->data_0e);
        self->data_0e = NULL;
    }
    self->count_04 = 0;
    self->alloc_bytes_0a = 0;
}

/* `1008:b7ec` — `CCollection_Clone_1008_b7ec` */
SBCollection *CCollection_Clone_1008_b7ec(const SBCollection *self) {
    SBCollection *copy;
    if (!self) {
        return NULL;
    }
    copy = CCollection_Ctor_1008_b760(NULL, self->elem_size_06);
    if (copy) {
        copy->vtable_tag_00 = self->vtable_tag_00;
        CCollection_CopyDataFrom_1008_b844(copy, self);
    }
    return copy;
}

/* `1008:b844` — `CCollection_CopyDataFrom_1008_b844` */
void CCollection_CopyDataFrom_1008_b844(SBCollection *self, const SBCollection *src) {
    if (!self || !src) {
        return;
    }
    self->elem_size_06 = src->elem_size_06;
    self->chunk_grow_08 = src->chunk_grow_08;
    self->is_sorted_12 = src->is_sorted_12;
    CCollection_SetArraySize_1008_b920(self, src->count_04);
    if (src->count_04 > 0 && self->data_0e && src->data_0e) {
        memcpy(self->data_0e, src->data_0e, (size_t)src->count_04 * (size_t)src->elem_size_06);
    }
}

/* `1008:b89c` — `CCollection_Dispose_1008_b89c` */
void CCollection_Dispose_1008_b89c(SBCollection *self) {
    if (!self) {
        return;
    }
    CCollection_Dtor_1008_b7b8(self);
}

/* `1008:b8bc` — `CCollection_GetCount_1008_b8bc` */
int16_t CCollection_GetCount_1008_b8bc(const SBCollection *self) {
    return self ? self->count_04 : 0;
}

/* `1008:b8ca` — `CCollection_IsEmpty_1008_b8ca` */
int CCollection_IsEmpty_1008_b8ca(const SBCollection *self) {
    return (!self || self->count_04 == 0) ? 1 : 0;
}

/* `1008:b8da` — `CCollection_Lock_1008_b8da` */
int16_t CCollection_Lock_1008_b8da(SBCollection *self, uint8_t lock_flag) {
    int16_t prev;
    if (!self) {
        return 0;
    }
    prev = self->lock_depth_0c;
    if (lock_flag) {
        self->lock_depth_0c++;
    } else if (self->lock_depth_0c > 0) {
        self->lock_depth_0c--;
    }
    return prev;
}

/* `1008:b994` — `CCollection_ResizeBuffer_1008_b994` */
void CCollection_ResizeBuffer_1008_b994(SBCollection *self, int16_t byte_size) {
    uint8_t *new_buf;
    if (!self) {
        return;
    }
    if (byte_size <= 0) {
        free(self->data_0e);
        self->data_0e = NULL;
        self->alloc_bytes_0a = 0;
        return;
    }
    new_buf = (uint8_t *)realloc(self->data_0e, (size_t)byte_size);
    if (new_buf) {
        if (byte_size > self->alloc_bytes_0a) {
            memset(new_buf + self->alloc_bytes_0a, 0, (size_t)(byte_size - self->alloc_bytes_0a));
        }
        self->data_0e = new_buf;
        self->alloc_bytes_0a = byte_size;
    }
}

/* `1008:b920` — `CCollection_SetArraySize_1008_b920` */
void CCollection_SetArraySize_1008_b920(SBCollection *self, int16_t new_count) {
    int32_t needed_bytes;
    int16_t grow;
    if (!self || new_count < 0) {
        return;
    }
    needed_bytes = (int32_t)new_count * (int32_t)self->elem_size_06;
    grow = (self->chunk_grow_08 > 0) ? self->chunk_grow_08 : self->elem_size_06;
    if (needed_bytes > self->alloc_bytes_0a) {
        int16_t target_bytes = (int16_t)(((needed_bytes + grow - 1) / grow) * grow);
        CCollection_ResizeBuffer_1008_b994(self, target_bytes);
    } else if (new_count == 0) {
        CCollection_ResizeBuffer_1008_b994(self, 0);
    }
    self->count_04 = new_count;
}

/* `1008:ba14` — `CCollection_InsertAt_1008_ba14` (1-based index) */
void CCollection_InsertAt_1008_ba14(SBCollection *self, int16_t index_1based, const void *elem_ptr) {
    int16_t old_count;
    size_t elem_sz;
    if (!self || !elem_ptr) {
        return;
    }
    old_count = self->count_04;
    if (index_1based < 1) {
        index_1based = 1;
    } else if (index_1based > old_count + 1) {
        index_1based = (int16_t)(old_count + 1);
    }
    CCollection_SetArraySize_1008_b920(self, (int16_t)(old_count + 1));
    elem_sz = (size_t)self->elem_size_06;
    if (index_1based <= old_count) {
        memmove(self->data_0e + (size_t)index_1based * elem_sz,
                self->data_0e + (size_t)(index_1based - 1) * elem_sz,
                (size_t)(old_count - index_1based + 1) * elem_sz);
    }
    memcpy(self->data_0e + (size_t)(index_1based - 1) * elem_sz, elem_ptr, elem_sz);
}

/* `1008:ba9c` — `CCollection_DeleteAt_1008_ba9c` (1-based index) */
void CCollection_DeleteAt_1008_ba9c(SBCollection *self, int16_t index_1based, int16_t count) {
    int16_t old_count;
    size_t elem_sz;
    if (!self || count <= 0 || index_1based < 1 || index_1based > self->count_04) {
        return;
    }
    old_count = self->count_04;
    if (index_1based + count - 1 > old_count) {
        count = (int16_t)(old_count - index_1based + 1);
    }
    elem_sz = (size_t)self->elem_size_06;
    if (index_1based + count <= old_count) {
        memmove(self->data_0e + (size_t)(index_1based - 1) * elem_sz,
                self->data_0e + (size_t)(index_1based - 1 + count) * elem_sz,
                (size_t)(old_count - (index_1based - 1 + count)) * elem_sz);
    }
    CCollection_SetArraySize_1008_b920(self, (int16_t)(old_count - count));
}

/* `1008:bb0c` — `CCollection_GetElements_1008_bb0c` */
void CCollection_GetElements_1008_bb0c(const SBCollection *self,
                                       int16_t index_1based,
                                       int16_t count,
                                       void *out_buf) {
    size_t elem_sz;
    if (!self || !out_buf || count <= 0 || index_1based < 1 || index_1based > self->count_04) {
        return;
    }
    if (index_1based + count - 1 > self->count_04) {
        count = (int16_t)(self->count_04 - index_1based + 1);
    }
    elem_sz = (size_t)self->elem_size_06;
    memcpy(out_buf, self->data_0e + (size_t)(index_1based - 1) * elem_sz, (size_t)count * elem_sz);
}

/* `1008:bb76` — `CCollection_SetElements_1008_bb76` */
void CCollection_SetElements_1008_bb76(SBCollection *self,
                                       int16_t index_1based,
                                       int16_t count,
                                       const void *in_buf) {
    size_t elem_sz;
    if (!self || !in_buf || count <= 0 || index_1based < 1 || index_1based > self->count_04) {
        return;
    }
    if (index_1based + count - 1 > self->count_04) {
        count = (int16_t)(self->count_04 - index_1based + 1);
    }
    elem_sz = (size_t)self->elem_size_06;
    memcpy(self->data_0e + (size_t)(index_1based - 1) * elem_sz, in_buf, (size_t)count * elem_sz);
}

/* `1008:bbe4` — `CCollection_RemoveAll_1008_bbe4` */
void CCollection_RemoveAll_1008_bbe4(SBCollection *self) {
    if (!self) {
        return;
    }
    CCollection_SetArraySize_1008_b920(self, 0);
}

/* `1008:bc16` — `CCollection_GetElementPtr_1008_bc16` */
void *CCollection_GetElementPtr_1008_bc16(const SBCollection *self, int16_t index_1based) {
    if (!self || !self->data_0e || index_1based < 1 || index_1based > self->count_04) {
        return NULL;
    }
    return self->data_0e + (size_t)(index_1based - 1) * (size_t)self->elem_size_06;
}

/* `1008:bc5c` — `CList_Ctor_1008_bc5c` */
SBCollection *CList_Ctor_1008_bc5c(SBCollection *self, int16_t elem_size) {
    self = CCollection_Ctor_1008_b760(self, elem_size);
    if (self) {
        self->vtable_tag_00 = 0x10080904u;
    }
    return self;
}

/* `1008:bc96` — `CList_Dtor_1008_bc96` */
void CList_Dtor_1008_bc96(SBCollection *self) {
    CCollection_Dtor_1008_b7b8(self);
}

/* `1008:bcc2` — `CList_GetAt_1008_bcc2` */
void CList_GetAt_1008_bcc2(const SBCollection *self, int16_t index_1based, void *out_elem) {
    CCollection_GetElements_1008_bb0c(self, index_1based, 1, out_elem);
}

/* `1008:bce2` — `CList_SetAt_1008_bce2` */
void CList_SetAt_1008_bce2(SBCollection *self, int16_t index_1based, const void *in_elem) {
    CCollection_SetElements_1008_bb76(self, index_1based, 1, in_elem);
}

/* `1008:bd02` — `CList_InsertBefore_1008_bd02` */
void CList_InsertBefore_1008_bd02(SBCollection *self, int16_t index_1based, const void *in_elem) {
    CCollection_InsertAt_1008_ba14(self, index_1based, in_elem);
}

/* `1008:bd5c` — `CList_DeleteOne_1008_bd5c` */
void CList_DeleteOne_1008_bd5c(SBCollection *self, int16_t index_1based) {
    CCollection_DeleteAt_1008_ba9c(self, index_1based, 1);
}

/* `1008:bdb6` — `CList_Swap_1008_bdb6` */
void CList_Swap_1008_bdb6(SBCollection *self, int16_t idx_a_1based, int16_t idx_b_1based) {
    uint8_t tmp[64];
    void *pa;
    void *pb;
    size_t sz;
    if (!self || idx_a_1based == idx_b_1based) {
        return;
    }
    pa = CCollection_GetElementPtr_1008_bc16(self, idx_a_1based);
    pb = CCollection_GetElementPtr_1008_bc16(self, idx_b_1based);
    sz = (size_t)self->elem_size_06;
    if (!pa || !pb || sz > sizeof(tmp)) {
        return;
    }
    memcpy(tmp, pa, sz);
    memcpy(pa, pb, sz);
    memcpy(pb, tmp, sz);
}

/* `1008:be6a` — `CList_Move_1008_be6a` */
void CList_Move_1008_be6a(SBCollection *self, int16_t from_1based, int16_t to_1based) {
    uint8_t tmp[64];
    void *p;
    size_t sz;
    if (!self || from_1based == to_1based) {
        return;
    }
    p = CCollection_GetElementPtr_1008_bc16(self, from_1based);
    sz = (size_t)self->elem_size_06;
    if (!p || sz > sizeof(tmp)) {
        return;
    }
    memcpy(tmp, p, sz);
    CList_DeleteOne_1008_bd5c(self, from_1based);
    CList_InsertBefore_1008_bd02(self, to_1based, tmp);
}

/* `1008:bed0` — `CList_Sort_1008_bed0` */
void CList_Sort_1008_bed0(SBCollection *self, SBCollectionCompareFn cmp) {
    int16_t i;
    int16_t j;
    if (!self || !cmp || self->count_04 <= 1) {
        return;
    }
    for (i = 1; i < self->count_04; ++i) {
        for (j = (int16_t)(i + 1); j <= self->count_04; ++j) {
            const void *pa = CCollection_GetElementPtr_1008_bc16(self, i);
            const void *pb = CCollection_GetElementPtr_1008_bc16(self, j);
            if (cmp(pa, pb) > 0) {
                CList_Swap_1008_bdb6(self, i, j);
            }
        }
    }
    self->is_sorted_12 = 1;
}

/* `1008:bf52` — `CList_LinearSearch_1008_bf52` */
int16_t CList_LinearSearch_1008_bf52(const SBCollection *self,
                                     const void *target_elem,
                                     SBCollectionCompareFn cmp) {
    int16_t i;
    if (!self || !target_elem) {
        return 0;
    }
    for (i = 1; i <= self->count_04; ++i) {
        const void *p = CCollection_GetElementPtr_1008_bc16(self, i);
        if (cmp) {
            if (cmp(p, target_elem) == 0) {
                return i;
            }
        } else if (memcmp(p, target_elem, (size_t)self->elem_size_06) == 0) {
            return i;
        }
    }
    return 0;
}

/* `1008:bfc6` — `CList_BinarySearch_1008_bfc6` */
int16_t CList_BinarySearch_1008_bfc6(const SBCollection *self,
                                     const void *target_elem,
                                     SBCollectionCompareFn cmp) {
    int16_t lo;
    int16_t hi;
    if (!self || !target_elem) {
        return 0;
    }
    lo = 1;
    hi = self->count_04;
    while (lo <= hi) {
        int16_t mid = (int16_t)((lo + hi) / 2);
        const void *p = CCollection_GetElementPtr_1008_bc16(self, mid);
        int c = cmp ? cmp(p, target_elem) : memcmp(p, target_elem, (size_t)self->elem_size_06);
        if (c == 0) {
            return mid;
        }
        if (c < 0) {
            lo = (int16_t)(mid + 1);
        } else {
            hi = (int16_t)(mid - 1);
        }
    }
    return 0;
}

/* `1008:c02a` — `COrderedList_Ctor_1008_c02a` */
SBCollection *COrderedList_Ctor_1008_c02a(SBCollection *self, int16_t elem_size) {
    self = CList_Ctor_1008_bc5c(self, elem_size);
    if (self) {
        self->vtable_tag_00 = 0x10080b08u;
        self->is_sorted_12 = 1;
    }
    return self;
}

/* `1008:c060` — `COrderedList_Dtor_1008_c060` */
void COrderedList_Dtor_1008_c060(SBCollection *self) {
    CList_Dtor_1008_bc96(self);
}

/* `1008:c08c` — `COrderedList_CompareElems_1008_c08c` */
int16_t COrderedList_CompareElems_1008_c08c(const SBCollection *self, const void *a, const void *b) {
    int c;
    if (!self || !a || !b) {
        return 0;
    }
    c = memcmp(a, b, (size_t)self->elem_size_06);
    return (c < 0) ? -1 : ((c > 0) ? 1 : 0);
}

/* `1008:c0a8` — `COrderedList_FindIndex_1008_c0a8` */
int16_t COrderedList_FindIndex_1008_c0a8(const SBCollection *self, const void *elem) {
    return CList_LinearSearch_1008_bf52(self, elem, NULL);
}

/* `1008:c0d6` — `COrderedList_InsertSorted_1008_c0d6` */
void COrderedList_InsertSorted_1008_c0d6(SBCollection *self, const void *elem) {
    int16_t i;
    if (!self || !elem) {
        return;
    }
    for (i = 1; i <= self->count_04; ++i) {
        const void *p = CCollection_GetElementPtr_1008_bc16(self, i);
        if (COrderedList_CompareElems_1008_c08c(self, elem, p) < 0) {
            break;
        }
    }
    CCollection_InsertAt_1008_ba14(self, i, elem);
}

/* `1008:c100` — `CObList_Ctor_1008_c100` */
SBCollection *CObList_Ctor_1008_c100(SBCollection *self) {
    self = COrderedList_Ctor_1008_c02a(self, (int16_t)sizeof(void *));
    if (self) {
        self->vtable_tag_00 = 0x10080a48u;
    }
    return self;
}

/* `1008:c11a` — `CObList_Dtor_1008_c11a` */
void CObList_Dtor_1008_c11a(SBCollection *self) {
    COrderedList_Dtor_1008_c060(self);
}

/* `1008:c146` — `CObList_ForEach_1008_c146` */
void CObList_ForEach_1008_c146(SBCollection *self, SBObListIterFn fn) {
    int16_t i;
    int16_t prev_lock;
    if (!self || !fn) {
        return;
    }
    prev_lock = CCollection_Lock_1008_b8da(self, 1);
    for (i = 1; i <= self->count_04; ++i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        if (obj) {
            fn(obj);
        }
    }
    self->lock_depth_0c = prev_lock;
}

/* `1008:c19e` — `CObList_ForEach1_1008_c19e` */
void CObList_ForEach1_1008_c19e(SBCollection *self, SBObListIter1Fn fn, void *ctx) {
    int16_t i;
    int16_t prev_lock;
    if (!self || !fn) {
        return;
    }
    prev_lock = CCollection_Lock_1008_b8da(self, 1);
    for (i = 1; i <= self->count_04; ++i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        if (obj) {
            fn(obj, ctx);
        }
    }
    self->lock_depth_0c = prev_lock;
}

/* `1008:c1fc` — `CObList_FirstThat_1008_c1fc` */
void *CObList_FirstThat_1008_c1fc(SBCollection *self, SBObListPredFn pred) {
    int16_t i;
    int16_t prev_lock;
    void *found = NULL;
    if (!self || !pred) {
        return NULL;
    }
    prev_lock = CCollection_Lock_1008_b8da(self, 1);
    for (i = 1; i <= self->count_04; ++i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        if (obj && pred(obj)) {
            found = obj;
            break;
        }
    }
    self->lock_depth_0c = prev_lock;
    return found;
}

/* `1008:c266` — `CObList_FirstThat1_1008_c266` */
void *CObList_FirstThat1_1008_c266(SBCollection *self, SBObListPred1Fn pred, void *ctx) {
    int16_t i;
    int16_t prev_lock;
    void *found = NULL;
    if (!self || !pred) {
        return NULL;
    }
    prev_lock = CCollection_Lock_1008_b8da(self, 1);
    for (i = 1; i <= self->count_04; ++i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        if (obj && pred(obj, ctx)) {
            found = obj;
            break;
        }
    }
    self->lock_depth_0c = prev_lock;
    return found;
}

/* `1008:c2d8` — `CObList_LastThat_1008_c2d8` */
void *CObList_LastThat_1008_c2d8(SBCollection *self, SBObListPredFn pred) {
    int16_t i;
    int16_t prev_lock;
    void *found = NULL;
    if (!self || !pred) {
        return NULL;
    }
    prev_lock = CCollection_Lock_1008_b8da(self, 1);
    for (i = self->count_04; i >= 1; --i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        if (obj && pred(obj)) {
            found = obj;
            break;
        }
    }
    self->lock_depth_0c = prev_lock;
    return found;
}

/* `1008:c346` — `CObList_LastThat1_1008_c346` */
void *CObList_LastThat1_1008_c346(SBCollection *self, SBObListPred1Fn pred, void *ctx) {
    int16_t i;
    int16_t prev_lock;
    void *found = NULL;
    if (!self || !pred) {
        return NULL;
    }
    prev_lock = CCollection_Lock_1008_b8da(self, 1);
    for (i = self->count_04; i >= 1; --i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        if (obj && pred(obj, ctx)) {
            found = obj;
            break;
        }
    }
    self->lock_depth_0c = prev_lock;
    return found;
}

/* `1008:c3ba` — `CObList_At_1008_c3ba` */
void *CObList_At_1008_c3ba(const SBCollection *self, int16_t index_1based) {
    void *obj = NULL;
    CCollection_GetElements_1008_bb0c(self, index_1based, 1, &obj);
    return obj;
}

/* `1008:c3e8` — `CObList_AtPut_1008_c3e8` */
void CObList_AtPut_1008_c3e8(SBCollection *self, int16_t index_1based, void *obj) {
    CCollection_SetElements_1008_bb76(self, index_1based, 1, &obj);
}

/* `1008:c40a` — `CObList_InsertAt_1008_c40a` */
void CObList_InsertAt_1008_c40a(SBCollection *self, int16_t index_1based, void *obj) {
    CCollection_InsertAt_1008_ba14(self, index_1based, &obj);
}

/* `1008:c46c` — `CObList_First_1008_c46c` */
void *CObList_First_1008_c46c(const SBCollection *self) {
    if (!self || self->count_04 == 0) {
        return NULL;
    }
    return CObList_At_1008_c3ba(self, 1);
}

/* `1008:c4a0` — `CObList_Last_1008_c4a0` */
void *CObList_Last_1008_c4a0(const SBCollection *self) {
    if (!self || self->count_04 == 0) {
        return NULL;
    }
    return CObList_At_1008_c3ba(self, self->count_04);
}

/* `1008:c4dc` — `CObList_Includes_1008_c4dc` */
int CObList_Includes_1008_c4dc(const SBCollection *self, const void *obj) {
    return (CObList_IndexOf_1008_c5cc(self, obj) != 0) ? 1 : 0;
}

/* `1008:c504` — `CObList_InsertFirst_1008_c504` */
void CObList_InsertFirst_1008_c504(SBCollection *self, void *obj) {
    CObList_InsertAt_1008_c40a(self, 1, obj);
}

/* `1008:c520` — `CObList_InsertLast_1008_c520` */
void CObList_InsertLast_1008_c520(SBCollection *self, void *obj) {
    if (!self) {
        return;
    }
    CObList_InsertAt_1008_c40a(self, (int16_t)(self->count_04 + 1), obj);
}

/* `1008:c554` — `CObList_Remove_1008_c554` */
void CObList_Remove_1008_c554(SBCollection *self, const void *obj) {
    int16_t idx = CObList_IndexOf_1008_c5cc(self, obj);
    if (idx > 0) {
        CObList_RemoveAt_1008_c59a(self, idx);
    }
}

/* `1008:c59a` — `CObList_RemoveAt_1008_c59a` */
void CObList_RemoveAt_1008_c59a(SBCollection *self, int16_t index_1based) {
    CCollection_DeleteAt_1008_ba9c(self, index_1based, 1);
}

/* `1008:c5cc` — `CObList_IndexOf_1008_c5cc` */
int16_t CObList_IndexOf_1008_c5cc(const SBCollection *self, const void *obj) {
    int16_t i;
    if (!self) {
        return 0;
    }
    for (i = 1; i <= self->count_04; ++i) {
        if (CObList_At_1008_c3ba(self, i) == obj) {
            return i;
        }
    }
    return 0;
}

/* `1008:c62a` — `CObList_DisposeAll_1008_c62a` */
void CObList_DisposeAll_1008_c62a(SBCollection *self) {
    if (!self) {
        return;
    }
    CCollection_RemoveAll_1008_bbe4(self);
}

/* `1008:c668` — `CObList_DisposeAndFree_1008_c668` */
void CObList_DisposeAndFree_1008_c668(SBCollection *self) {
    int16_t i;
    if (!self) {
        return;
    }
    for (i = 1; i <= self->count_04; ++i) {
        void *obj = CObList_At_1008_c3ba(self, i);
        free(obj);
    }
    CCollection_RemoveAll_1008_bbe4(self);
}

/* `1008:c6a8` — `CObList_ComparePtrs_1008_c6a8` */
int16_t CObList_ComparePtrs_1008_c6a8(const void *a, const void *b) {
    uintptr_t pa = (uintptr_t)a;
    uintptr_t pb = (uintptr_t)b;
    return (pa < pb) ? -1 : ((pa > pb) ? 1 : 0);
}

/* `1008:c6d4` — `CObList_MoveToFront_1008_c6d4` */
void CObList_MoveToFront_1008_c6d4(SBCollection *self, void *obj) {
    int16_t idx = CObList_IndexOf_1008_c5cc(self, obj);
    if (idx > 1) {
        CList_Move_1008_be6a(self, idx, 1);
    }
}

/* `1008:c71a` — `CObList_MoveToBack_1008_c71a` */
void CObList_MoveToBack_1008_c71a(SBCollection *self, void *obj) {
    int16_t idx = CObList_IndexOf_1008_c5cc(self, obj);
    if (idx > 0 && idx < self->count_04) {
        CList_Move_1008_be6a(self, idx, self->count_04);
    }
}

/* `1010:036c` — `CObList_New_1010_036c` */
SBCollection *CObList_New_1010_036c(void) {
    return CObList_Ctor_1008_c100(NULL);
}

/* `1010:1246` — `CObList_CallVirtual_1010_1246` */
void CObList_CallVirtual_1010_1246(SBCollection *self, uint16_t vtable_slot_offset) {
    int16_t i;
    if (!self) {
        return;
    }
    for (i = 1; i <= self->count_04; ++i) {
        SBEventTarget *target = (SBEventTarget *)CObList_At_1008_c3ba(self, i);
        if (target && vtable_slot_offset == 0x20u) {
            CEventTarget_Idle_1008_b758(target);
        }
    }
}

/* =========================================================================
 * 2. Event Dispatcher (`CEventDispatcherBase_1008_0b84` & `CEventDispatcher_1008_7f84`)
 * ========================================================================= */

/* `1010:3a28` — `CEventDispatcherBase_Ctor_1010_3a28` */
SBEventDispatcher *CEventDispatcherBase_Ctor_1010_3a28(SBEventDispatcher *self) {
    if (!self) {
        self = (SBEventDispatcher *)calloc(1, sizeof(SBEventDispatcher));
    } else {
        memset(self, 0, sizeof(SBEventDispatcher));
    }
    if (!self) {
        return NULL;
    }
    CEventTarget_Ctor_1008_b6f8(&self->base_00);
    self->base_00.vtable_tag_00 = 0x10080b84u;
    self->idle_tasks_08 = CObList_New_1010_036c();
    self->event_mask_18 = 0xFFFFu;
    self->sleep_ticks_1a = 1;
    self->double_click_ticks_46 = 30;
    self->arrow_cursor_4a = sb_win16_LoadCursor(0, 0x7F00u);
    self->wait_cursor_4c = sb_win16_LoadCursor(0, 0x7F02u);
    self->active_cursor_4e = self->arrow_cursor_4a;
    g_DAT_1060_0836_dispatcher = self;
    return self;
}

/* `1010:3ae8` — `CEventDispatcherBase_Dtor_1010_3ae8` */
void CEventDispatcherBase_Dtor_1010_3ae8(SBEventDispatcher *self) {
    if (!self) {
        return;
    }
    if (self->idle_tasks_08) {
        CObList_Dtor_1008_c11a(self->idle_tasks_08);
        free(self->idle_tasks_08);
        self->idle_tasks_08 = NULL;
    }
    if (g_DAT_1060_0836_dispatcher == self) {
        g_DAT_1060_0836_dispatcher = NULL;
    }
    CEventTarget_Dtor_1008_b714(&self->base_00);
}

/* `1010:3b2c` — `CEventDispatcherBase_Init_1010_3b2c` */
void CEventDispatcherBase_Init_1010_3b2c(SBEventDispatcher *self) {
    if (!self) {
        return;
    }
    if (!self->idle_tasks_08) {
        self->idle_tasks_08 = CObList_New_1010_036c();
    }
    sb_win16_SetCursor(self->arrow_cursor_4a);
}

/* `1010:41a8` — `CEventDispatcherBase_UpdateModifiers_1010_41a8` */
void CEventDispatcherBase_UpdateModifiers_1010_41a8(SBEventDispatcher *self) {
    uint16_t mods = 0;
    if (!self) {
        return;
    }
    if (sb_win16_GetKeyState(0x10) < 0) { /* VK_SHIFT */
        mods |= 0x0200u;
    }
    if (sb_win16_GetKeyState(0x11) < 0) { /* VK_CONTROL */
        mods |= 0x0100u;
    }
    if (sb_win16_GetKeyState(0x12) < 0) { /* VK_MENU (Alt) */
        mods |= 0x0800u;
    }
    if (self->button_down_44) {
        mods |= 0x0080u;
    }
    self->modifiers_30 = mods;
}

/* `1010:3d98` — `CEventDispatcherBase_HandleMouseDown_1010_3d98` */
void CEventDispatcherBase_HandleMouseDown_1010_3d98(SBEventDispatcher *self,
                                                    SB_HWND hwnd,
                                                    SB_POINT16 pt,
                                                    uint16_t flags) {
    uint32_t now_ticks;
    SBWindow *win;
    (void)flags;
    if (!self) {
        return;
    }
    now_ticks = CTimer60Hz_GetTicks_1010_44ec(g_DAT_1060_0a1e_timer60Hz);
    self->button_down_44 = 1;
    CEventDispatcherBase_UpdateModifiers_1010_41a8(self);

    if (now_ticks - self->last_click_time_32 <= self->double_click_ticks_46 &&
        abs(pt.x - self->last_click_pt_36.x) <= 4 &&
        abs(pt.y - self->last_click_pt_36.y) <= 4) {
        self->click_count_3a++;
    } else {
        self->click_count_3a = 1;
    }
    self->last_click_time_32 = now_ticks;
    self->last_click_pt_36 = pt;
    self->last_mouse_pt_3c = pt;

    win = CWindow_FromHwnd_1010_59c2(hwnd);
    if (!win && g_DAT_1060_0810_desktop) {
        win = g_DAT_1060_0810_desktop->top_window_3c;
    }
    if (win) {
        CView_DispatchClick_1010_493c(&win->view_00, pt, self->modifiers_30, self->click_count_3a);
    } else if (g_DAT_1060_0810_desktop) {
        CView_DispatchClick_1010_493c(&g_DAT_1060_0810_desktop->view_00, pt, self->modifiers_30, self->click_count_3a);
    }
}

/* `1010:3f02` — `CEventDispatcherBase_HandleMouseUp_1010_3f02` */
void CEventDispatcherBase_HandleMouseUp_1010_3f02(SBEventDispatcher *self,
                                                  SB_HWND hwnd,
                                                  SB_POINT16 pt,
                                                  uint16_t flags) {
    (void)hwnd;
    (void)flags;
    if (!self) {
        return;
    }
    self->button_down_44 = 0;
    self->last_mouse_pt_3c = pt;
    CEventDispatcherBase_UpdateModifiers_1010_41a8(self);
}

/* `1010:3f7c` — `CEventDispatcherBase_HandleMouseMove_1010_3f7c` */
void CEventDispatcherBase_HandleMouseMove_1010_3f7c(SBEventDispatcher *self,
                                                    SB_HWND hwnd,
                                                    SB_POINT16 pt,
                                                    uint16_t flags) {
    SBWindow *win;
    (void)flags;
    if (!self) {
        return;
    }
    self->last_mouse_pt_3c = pt;
    win = CWindow_FromHwnd_1010_59c2(hwnd);
    if (win) {
        CView_AdjustCursor_1010_4f78(&win->view_00, pt);
    }
}

/* `1010:4008` — `CEventDispatcherBase_HandleKeyDown_1010_4008` */
void CEventDispatcherBase_HandleKeyDown_1010_4008(SBEventDispatcher *self, uint16_t vk, uint32_t lParam) {
    SBEventTarget *target;
    (void)lParam;
    if (!self) {
        return;
    }
    CEventDispatcherBase_UpdateModifiers_1010_41a8(self);
    target = self->focus_target_0c;
    if (!target && g_DAT_1060_0828_focusedView) {
        target = &g_DAT_1060_0828_focusedView->base_00;
    }
    if (!target) {
        target = &self->base_00;
    }
    CEventTarget_HandleKey_1008_b74e(target, vk, self->modifiers_30);
}

/* `1010:40a4` — `CEventDispatcherBase_HandleChar_1010_40a4` */
void CEventDispatcherBase_HandleChar_1010_40a4(SBEventDispatcher *self, uint16_t ch, uint32_t lParam) {
    SBEventTarget *target;
    (void)lParam;
    if (!self) {
        return;
    }
    CEventDispatcherBase_UpdateModifiers_1010_41a8(self);
    target = self->focus_target_0c;
    if (!target && g_DAT_1060_0828_focusedView) {
        target = &g_DAT_1060_0828_focusedView->base_00;
    }
    if (!target) {
        target = &self->base_00;
    }
    CEventTarget_HandleKey_1008_b74e(target, ch, self->modifiers_30);
}

/* `1010:4120` — `CEventDispatcherBase_DoIdle_1010_4120` */
void CEventDispatcherBase_DoIdle_1010_4120(SBEventDispatcher *self) {
    if (!self) {
        return;
    }
    self->last_idle_time_40 = CTimer60Hz_GetTicks_1010_44ec(g_DAT_1060_0a1e_timer60Hz);
    if (self->idle_tasks_08) {
        CObList_CallVirtual_1010_1246(self->idle_tasks_08, 0x20u);
    }
    if (g_DAT_1060_0a2e_idleList) {
        CObList_CallVirtual_1010_1246(g_DAT_1060_0a2e_idleList, 0x20u);
    }
}

/* `1010:3c64` — `CEventDispatcherBase_DispatchWinMessage_1010_3c64` */
void CEventDispatcherBase_DispatchWinMessage_1010_3c64(SBEventDispatcher *self, const SB_MSG16 *msg) {
    SB_POINT16 pt;
    if (!self || !msg) {
        return;
    }
    pt.x = (int16_t)(msg->lParam & 0xFFFFu);
    pt.y = (int16_t)((msg->lParam >> 16) & 0xFFFFu);

    switch (msg->message) {
        case SB_WM_LBUTTONDOWN:
        case SB_WM_RBUTTONDOWN:
        case SB_WM_MBUTTONDOWN:
        case SB_WM_LBUTTONDBLCLK:
            CEventDispatcherBase_HandleMouseDown_1010_3d98(self, msg->hwnd, pt, msg->wParam);
            break;
        case SB_WM_LBUTTONUP:
        case SB_WM_RBUTTONUP:
        case SB_WM_MBUTTONUP:
            CEventDispatcherBase_HandleMouseUp_1010_3f02(self, msg->hwnd, pt, msg->wParam);
            break;
        case SB_WM_MOUSEMOVE:
            CEventDispatcherBase_HandleMouseMove_1010_3f7c(self, msg->hwnd, pt, msg->wParam);
            break;
        case SB_WM_KEYDOWN:
        case SB_WM_SYSKEYDOWN:
            CEventDispatcherBase_HandleKeyDown_1010_4008(self, msg->wParam, msg->lParam);
            break;
        case SB_WM_CHAR:
            CEventDispatcherBase_HandleChar_1010_40a4(self, msg->wParam, msg->lParam);
            break;
        default:
            CWindow_WndProc_1010_5a18(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            break;
    }
}

/* `1010:3bb2` — `CEventDispatcherBase_ProcessOneEvent_1010_3bb2` */
int CEventDispatcherBase_ProcessOneEvent_1010_3bb2(SBEventDispatcher *self) {
    if (!self) {
        return 0;
    }
    if (sb_win16_PeekMessage(&self->current_msg_1e, 0, 0, 0, SB_PM_REMOVE)) {
        sb_win16_TranslateMessage(&self->current_msg_1e);
        CEventDispatcherBase_DispatchWinMessage_1010_3c64(self, &self->current_msg_1e);
        return 1;
    }
    CEventDispatcherBase_DoIdle_1010_4120(self);
    return 0;
}

/* `1010:4236` — `CEventDispatcherBase_AddIdleTask_1010_4236` */
void CEventDispatcherBase_AddIdleTask_1010_4236(SBEventDispatcher *self, SBEventTarget *task) {
    if (!self || !task || !self->idle_tasks_08) {
        return;
    }
    if (!CObList_Includes_1008_c4dc(self->idle_tasks_08, task)) {
        CObList_InsertLast_1008_c520(self->idle_tasks_08, task);
    }
}

/* `1010:4268` — `CEventDispatcherBase_RemoveIdleTask_1010_4268` */
void CEventDispatcherBase_RemoveIdleTask_1010_4268(SBEventDispatcher *self, SBEventTarget *task) {
    if (!self || !task || !self->idle_tasks_08) {
        return;
    }
    CObList_Remove_1008_c554(self->idle_tasks_08, task);
}

/* `1010:429a` — `CEventDispatcherBase_SetWaitCursor_1010_429a` */
void CEventDispatcherBase_SetWaitCursor_1010_429a(SBEventDispatcher *self, uint8_t wait_on) {
    if (!self) {
        return;
    }
    self->active_cursor_4e = wait_on ? self->wait_cursor_4c : self->arrow_cursor_4a;
    sb_win16_SetCursor(self->active_cursor_4e);
}

/* `1010:42ce` — `CEventDispatcherBase_FlushEvents_1010_42ce` */
void CEventDispatcherBase_FlushEvents_1010_42ce(SBEventDispatcher *self) {
    SB_MSG16 msg;
    (void)self;
    while (sb_win16_PeekMessage(&msg, 0, 0, 0, SB_PM_REMOVE)) {
        /* drain pending messages */
    }
}

/* `1008:7c02` — `CEventDispatcher_Init_1008_7c02` */
SBEventDispatcher *CEventDispatcher_Init_1008_7c02(SBEventDispatcher *self) {
    self = CEventDispatcherBase_Ctor_1010_3a28(self);
    if (self) {
        self->base_00.vtable_tag_00 = 0x10087f84u;
        self->custom_flag_58 = 1;
        self->cursor_locked_59 = 0;
        self->custom_cursor_5a = 0;
        self->custom_cursor_id_5e = 0;
    }
    return self;
}

/* `1008:7c40` — `CEventDispatcher_SetCustomCursor_1008_7c40` */
void CEventDispatcher_SetCustomCursor_1008_7c40(SBEventDispatcher *self, uint16_t curs_res_id) {
    if (!self || self->cursor_locked_59) {
        return;
    }
    self->custom_cursor_id_5e = curs_res_id;
    self->custom_cursor_5a = sb_win16_LoadCursor(g_DAT_1060_1d58_hInstance, curs_res_id);
    self->active_cursor_4e = self->custom_cursor_5a;
    sb_win16_SetCursor(self->custom_cursor_5a);
}

/* `1008:7c5c` — `CEventDispatcher_RestoreArrowCursor_1008_7c5c` */
void CEventDispatcher_RestoreArrowCursor_1008_7c5c(SBEventDispatcher *self) {
    if (!self) {
        return;
    }
    self->active_cursor_4e = self->arrow_cursor_4a;
    sb_win16_SetCursor(self->arrow_cursor_4a);
}

/* `1020:2534` — `CEventDispatcher_IsButtonDown_1020_2534` */
uint8_t CEventDispatcher_IsButtonDown_1020_2534(const SBEventDispatcher *self) {
    return self ? self->button_down_44 : 0;
}

/* `1020:2552` — `CEventDispatcher_GetMousePos_1020_2552` */
void CEventDispatcher_GetMousePos_1020_2552(const SBEventDispatcher *self, SB_POINT16 *out_pt) {
    if (!out_pt) {
        return;
    }
    if (self) {
        *out_pt = self->last_mouse_pt_3c;
    } else {
        out_pt->x = 0;
        out_pt->y = 0;
    }
}

/* =========================================================================
 * 3. View & Desktop Hierarchy (`CView_1008_1aac` & `CDesktop_1008_7d10`)
 * ========================================================================= */

/* `1010:15f0` — `CView_Ctor_1010_15f0` */
SBView *CView_Ctor_1010_15f0(SBView *self,
                             SBView *superview,
                             SBEventTarget *next_handler,
                             int16_t loc_x,
                             int16_t loc_y,
                             int16_t width,
                             int16_t height,
                             uint8_t sizing_flags) {
    if (!self) {
        self = (SBView *)calloc(1, sizeof(SBView));
    } else {
        memset(self, 0, sizeof(SBView));
    }
    if (!self) {
        return NULL;
    }
    CEventTarget_Ctor_1008_b6f8(&self->base_00);
    self->base_00.vtable_tag_00 = 0x10081aacu;
    self->base_00.next_handler_04 = next_handler ? next_handler : (superview ? &superview->base_00 : NULL);
    self->subviews_08 = NULL;
    self->superview_0c = NULL;
    self->loc_x_10 = loc_x;
    self->loc_y_12 = loc_y;
    self->width_14 = width;
    self->height_16 = height;
    self->scroll_x_18 = 0;
    self->scroll_y_1a = 0;
    self->sizing_flags_1c = sizing_flags;
    self->visible_1d = 1;
    self->active_1e = 1;
    self->wants_clicks_1f = 1;
    self->pane_id_20 = 0;
    if (superview) {
        CView_AddSubview_1010_19a4(superview, self);
    }
    return self;
}

/* `1010:16d4` — `CView_Dtor_1010_16d4` */
void CView_Dtor_1010_16d4(SBView *self) {
    if (!self) {
        return;
    }
    if (self->superview_0c) {
        CView_RemoveSubview_1010_1a12(self->superview_0c, self);
    }
    if (self->subviews_08) {
        CObList_Dtor_1008_c11a(self->subviews_08);
        free(self->subviews_08);
        self->subviews_08 = NULL;
    }
    if (self->border_pen_2c) {
        sb_win16_DeleteObject(self->border_pen_2c);
        self->border_pen_2c = 0;
    }
    if (g_DAT_1060_0828_focusedView == self) {
        g_DAT_1060_0828_focusedView = NULL;
    }
    CEventTarget_Dtor_1008_b714(&self->base_00);
}

/* `1010:1732` — `CView_Dispose_1010_1732` */
void CView_Dispose_1010_1732(SBView *self) {
    CView_Dtor_1010_16d4(self);
}

/* `1010:188e` — `CView_InitFromStream_1010_188e`
 *
 * Parses a Bright Star `'Pane'` (`0x03f1`) binary stream:
 *   +0x00: int16_tbe version / class tag
 *   +0x02: int32_tbe pane_id
 *   +0x06: int16_tbe width
 *   +0x08: int16_tbe height
 *   +0x0a: int16_tbe loc_x
 *   +0x0c: int16_tbe loc_y
 *   +0x0e: uint8_t   sizing_flags
 *   +0x0f: uint8_t   visible_flags
 */
void CView_InitFromStream_1010_188e(SBView *self,
                                    SBView *superview,
                                    SBEventTarget *next_handler,
                                    const uint8_t *pane_data,
                                    uint32_t data_len) {
    int16_t loc_x = 0;
    int16_t loc_y = 0;
    int16_t width = 640;
    int16_t height = 480;
    int32_t pane_id = 0;
    uint8_t sizing = 0;
    uint8_t vis = 1;

    if (!self) {
        return;
    }
    if (pane_data && data_len >= 16) {
        pane_id = (int32_t)read_be32(pane_data + 2);
        width = read_be16s(pane_data + 6);
        height = read_be16s(pane_data + 8);
        loc_x = read_be16s(pane_data + 10);
        loc_y = read_be16s(pane_data + 12);
        sizing = pane_data[14];
        vis = (uint8_t)((pane_data[15] != 0) ? 1 : 0);
        if (width <= 0 || width > 2048) {
            width = 640;
        }
        if (height <= 0 || height > 2048) {
            height = 480;
        }
    }
    CView_Ctor_1010_15f0(self, superview, next_handler, loc_x, loc_y, width, height, sizing);
    self->pane_id_20 = pane_id;
    self->visible_1d = vis;
}

/* `1010:1826` — `CView_InitFromPaneResource_1010_1826` */
SBView *CView_InitFromPaneResource_1010_1826(SBView *self,
                                             SBView *superview,
                                             SBEventTarget *next_handler,
                                             uint16_t pane_res_id) {
    SBResourceHandle *hPane;
    const uint8_t *ptr = NULL;
    uint32_t sz = 0;

    if (!self) {
        self = (SBView *)calloc(1, sizeof(SBView));
    }
    hPane = ResMgr_GetResource_1010_10f2(0x03f1u, "Pane", pane_res_id);
    if (hPane) {
        Handle_Lock_1010_00e8(hPane);
        ptr = (const uint8_t *)hPane->data_00;
        sz = hPane->size_08;
    }
    CView_InitFromStream_1010_188e(self, superview, next_handler, ptr, sz);
    if (self && self->pane_id_20 == 0) {
        self->pane_id_20 = (int32_t)pane_res_id;
    }
    if (hPane) {
        Handle_Unlock_1010_0122(hPane);
    }
    return self;
}

/* `1010:19a4` — `CView_AddSubview_1010_19a4` */
void CView_AddSubview_1010_19a4(SBView *self, SBView *subview) {
    if (!self || !subview) {
        return;
    }
    if (!self->subviews_08) {
        self->subviews_08 = CObList_New_1010_036c();
    }
    if (!CObList_Includes_1008_c4dc(self->subviews_08, subview)) {
        CObList_InsertLast_1008_c520(self->subviews_08, subview);
    }
    subview->superview_0c = self;
    if (!subview->base_00.next_handler_04) {
        subview->base_00.next_handler_04 = &self->base_00;
    }
}

/* `1010:1a12` — `CView_RemoveSubview_1010_1a12` */
void CView_RemoveSubview_1010_1a12(SBView *self, SBView *subview) {
    if (!self || !subview || !self->subviews_08) {
        return;
    }
    CObList_Remove_1008_c554(self->subviews_08, subview);
    if (subview->superview_0c == self) {
        subview->superview_0c = NULL;
    }
}

/* `1010:1a66` — `CView_FindViewById_1010_1a66` */
SBView *CView_FindViewById_1010_1a66(SBView *self, int32_t pane_id) {
    int16_t i;
    if (!self) {
        return NULL;
    }
    if (self->pane_id_20 == pane_id) {
        return self;
    }
    if (self->subviews_08) {
        for (i = 1; i <= self->subviews_08->count_04; ++i) {
            SBView *child = (SBView *)CObList_At_1008_c3ba(self->subviews_08, i);
            SBView *found = CView_FindViewById_1010_1a66(child, pane_id);
            if (found) {
                return found;
            }
        }
    }
    return NULL;
}

/* `1010:1ae4` — `CView_Show_1010_1ae4` */
void CView_Show_1010_1ae4(SBView *self, uint8_t visible, uint8_t redraw) {
    if (!self) {
        return;
    }
    self->visible_1d = (uint8_t)(visible ? 1 : 0);
    if (redraw) {
        CView_InvalidateAll_1010_4e56(self);
    }
}

/* `1010:1b58` — `CView_Activate_1010_1b58` */
void CView_Activate_1010_1b58(SBView *self, uint8_t active) {
    int16_t i;
    if (!self) {
        return;
    }
    self->active_1e = (uint8_t)(active ? 1 : 0);
    if (self->subviews_08) {
        for (i = 1; i <= self->subviews_08->count_04; ++i) {
            SBView *child = (SBView *)CObList_At_1008_c3ba(self->subviews_08, i);
            CView_Activate_1010_1b58(child, active);
        }
    }
}

/* `1010:1bc8` — `CView_SetFrame_1010_1bc8` */
void CView_SetFrame_1010_1bc8(SBView *self, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t redraw) {
    if (!self) {
        return;
    }
    self->loc_x_10 = x;
    self->loc_y_12 = y;
    self->width_14 = w;
    self->height_16 = h;
    if (redraw) {
        CView_InvalidateAll_1010_4e56(self);
    }
}

/* `1010:1c52` — `CView_GetFrame_1010_1c52` */
void CView_GetFrame_1010_1c52(const SBView *self, SB_RECT16 *out_rc) {
    if (!out_rc) {
        return;
    }
    if (!self) {
        Rect_Set_1010_0fb6(out_rc, 0, 0, 0, 0);
        return;
    }
    Rect_Set_1010_0fb6(out_rc,
                       self->loc_x_10,
                       self->loc_y_12,
                       (int16_t)(self->loc_x_10 + self->width_14),
                       (int16_t)(self->loc_y_12 + self->height_16));
}

/* `1010:1c88` — `CView_GetBounds_1010_1c88` */
void CView_GetBounds_1010_1c88(const SBView *self, SB_RECT16 *out_rc) {
    if (!out_rc) {
        return;
    }
    if (!self) {
        Rect_Set_1010_0fb6(out_rc, 0, 0, 0, 0);
        return;
    }
    Rect_Set_1010_0fb6(out_rc,
                       self->scroll_x_18,
                       self->scroll_y_1a,
                       (int16_t)(self->scroll_x_18 + self->width_14),
                       (int16_t)(self->scroll_y_1a + self->height_16));
}

/* `1010:1cbe` — `CView_MoveTo_1010_1cbe` */
void CView_MoveTo_1010_1cbe(SBView *self, int16_t x, int16_t y, uint8_t redraw) {
    if (!self) {
        return;
    }
    CView_SetFrame_1010_1bc8(self, x, y, self->width_14, self->height_16, redraw);
}

/* `1010:1d12` — `CView_Resize_1010_1d12` */
void CView_Resize_1010_1d12(SBView *self, int16_t w, int16_t h, uint8_t redraw) {
    if (!self) {
        return;
    }
    CView_SetFrame_1010_1bc8(self, self->loc_x_10, self->loc_y_12, w, h, redraw);
}

/* `1010:1d74` — `CView_GetWindow_1010_1d74` */
SBWindow *CView_GetWindow_1010_1d74(const SBView *self) {
    const SBView *cur = self;
    while (cur) {
        if (cur->base_00.vtable_tag_00 == 0x10105942u) {
            return (SBWindow *)cur;
        }
        cur = cur->superview_0c;
    }
    return g_DAT_1060_0810_desktop ? g_DAT_1060_0810_desktop->top_window_3c : NULL;
}

/* `1010:1db0` — `CView_GetDesktop_1010_1db0` */
SBDesktop *CView_GetDesktop_1010_1db0(const SBView *self) {
    (void)self;
    return g_DAT_1060_0810_desktop;
}

/* `1010:1de8` — `CView_IsVisible_1010_1de8` */
int CView_IsVisible_1010_1de8(const SBView *self) {
    const SBView *cur = self;
    while (cur) {
        if (!cur->visible_1d) {
            return 0;
        }
        cur = cur->superview_0c;
    }
    return self ? 1 : 0;
}

/* `1010:473e` — `CView_LocalToSuper_1010_473e` */
void CView_LocalToSuper_1010_473e(const SBView *self, SB_POINT16 *io_pt) {
    if (!self || !io_pt) {
        return;
    }
    io_pt->x = (int16_t)(io_pt->x + self->loc_x_10 - self->scroll_x_18);
    io_pt->y = (int16_t)(io_pt->y + self->loc_y_12 - self->scroll_y_1a);
}

/* `1010:4768` — `CView_SuperToLocal_1010_4768` */
void CView_SuperToLocal_1010_4768(const SBView *self, SB_POINT16 *io_pt) {
    if (!self || !io_pt) {
        return;
    }
    io_pt->x = (int16_t)(io_pt->x - self->loc_x_10 + self->scroll_x_18);
    io_pt->y = (int16_t)(io_pt->y - self->loc_y_12 + self->scroll_y_1a);
}

/* `1010:45ea` — `CView_LocalToWindow_1010_45ea` */
void CView_LocalToWindow_1010_45ea(const SBView *self, SB_POINT16 *io_pt) {
    const SBView *cur = self;
    if (!io_pt) {
        return;
    }
    while (cur && cur->base_00.vtable_tag_00 != 0x10105942u && cur->base_00.vtable_tag_00 != 0x10087d10u) {
        CView_LocalToSuper_1010_473e(cur, io_pt);
        cur = cur->superview_0c;
    }
}

/* `1010:4638` — `CView_WindowToLocal_1010_4638` */
void CView_WindowToLocal_1010_4638(const SBView *self, SB_POINT16 *io_pt) {
    SB_POINT16 zero = {0, 0};
    if (!self || !io_pt) {
        return;
    }
    CView_LocalToWindow_1010_45ea(self, &zero);
    io_pt->x = (int16_t)(io_pt->x - zero.x);
    io_pt->y = (int16_t)(io_pt->y - zero.y);
}

/* `1010:4686` — `CView_LocalToGlobal_1010_4686` */
void CView_LocalToGlobal_1010_4686(const SBView *self, SB_POINT16 *io_pt) {
    SBWindow *win;
    if (!self || !io_pt) {
        return;
    }
    CView_LocalToWindow_1010_45ea(self, io_pt);
    win = CView_GetWindow_1010_1d74(self);
    if (win && win->hwnd_42) {
        sb_win16_ClientToScreen(win->hwnd_42, io_pt);
    }
}

/* `1010:46e2` — `CView_GlobalToLocal_1010_46e2` */
void CView_GlobalToLocal_1010_46e2(const SBView *self, SB_POINT16 *io_pt) {
    SBWindow *win;
    if (!self || !io_pt) {
        return;
    }
    win = CView_GetWindow_1010_1d74(self);
    if (win && win->hwnd_42) {
        sb_win16_ScreenToClient(win->hwnd_42, io_pt);
    }
    CView_WindowToLocal_1010_4638(self, io_pt);
}

/* `1010:4792` — `CView_FrameToWindR_1010_4792` */
void CView_FrameToWindR_1010_4792(const SBView *self, const SB_RECT16 *local_rc, SB_RECT16 *out_wind_rc) {
    SB_POINT16 tl;
    if (!local_rc || !out_wind_rc) {
        return;
    }
    tl.x = local_rc->left;
    tl.y = local_rc->top;
    CView_LocalToWindow_1010_45ea(self, &tl);
    Rect_Set_1010_0fb6(out_wind_rc,
                       tl.x,
                       tl.y,
                       (int16_t)(tl.x + (local_rc->right - local_rc->left)),
                       (int16_t)(tl.y + (local_rc->bottom - local_rc->top)));
}

/* `1010:47ec` — `CView_WindToFrameR_1010_47ec` */
void CView_WindToFrameR_1010_47ec(const SBView *self, const SB_RECT16 *wind_rc, SB_RECT16 *out_local_rc) {
    SB_POINT16 tl;
    if (!wind_rc || !out_local_rc) {
        return;
    }
    tl.x = wind_rc->left;
    tl.y = wind_rc->top;
    CView_WindowToLocal_1010_4638(self, &tl);
    Rect_Set_1010_0fb6(out_local_rc,
                       tl.x,
                       tl.y,
                       (int16_t)(tl.x + (wind_rc->right - wind_rc->left)),
                       (int16_t)(tl.y + (wind_rc->bottom - wind_rc->top)));
}

/* `1010:4846` — `CView_ContainsPoint_1010_4846` */
int CView_ContainsPoint_1010_4846(const SBView *self, SB_POINT16 super_pt) {
    SB_RECT16 frame;
    if (!self || !self->visible_1d) {
        return 0;
    }
    CView_GetFrame_1010_1c52(self, &frame);
    return sb_win16_PtInRect(&frame, super_pt);
}

/* `1010:48ac` — `CView_FindTargetView_1010_48ac` */
SBView *CView_FindTargetView_1010_48ac(SBView *self, SB_POINT16 super_pt) {
    SB_POINT16 local_pt;
    int16_t i;
    if (!self || !self->visible_1d || !CView_ContainsPoint_1010_4846(self, super_pt)) {
        return NULL;
    }
    local_pt = super_pt;
    CView_SuperToLocal_1010_4768(self, &local_pt);
    if (self->subviews_08) {
        for (i = self->subviews_08->count_04; i >= 1; --i) {
            SBView *child = (SBView *)CObList_At_1008_c3ba(self->subviews_08, i);
            SBView *hit = CView_FindTargetView_1010_48ac(child, local_pt);
            if (hit) {
                return hit;
            }
        }
    }
    return self->wants_clicks_1f ? self : NULL;
}

/* `1010:49d6` — `CView_HandleMouseDown_1010_49d6` */
void CView_HandleMouseDown_1010_49d6(SBView *self,
                                     SB_POINT16 local_pt,
                                     uint16_t modifiers,
                                     int16_t click_count) {
    (void)local_pt;
    (void)modifiers;
    (void)click_count;
    if (self) {
        g_DAT_1060_0828_focusedView = self;
    }
}

/* `1010:493c` — `CView_DispatchClick_1010_493c` */
int CView_DispatchClick_1010_493c(SBView *self,
                                  SB_POINT16 local_pt,
                                  uint16_t modifiers,
                                  int16_t click_count) {
    int16_t i;
    if (!self || !self->visible_1d || !self->active_1e) {
        return 0;
    }
    if (self->subviews_08) {
        for (i = self->subviews_08->count_04; i >= 1; --i) {
            SBView *child = (SBView *)CObList_At_1008_c3ba(self->subviews_08, i);
            if (child && CView_ContainsPoint_1010_4846(child, local_pt)) {
                SB_POINT16 child_pt = local_pt;
                CView_SuperToLocal_1010_4768(child, &child_pt);
                if (CView_DispatchClick_1010_493c(child, child_pt, modifiers, click_count)) {
                    return 1;
                }
            }
        }
    }
    if (self->wants_clicks_1f) {
        CView_HandleMouseDown_1010_49d6(self, local_pt, modifiers, click_count);
        return 1;
    }
    return 0;
}

/* `1010:4a0a` — `CView_PrepareDC_1010_4a0a` */
int CView_PrepareDC_1010_4a0a(SBView *self) {
    SBWindow *win;
    SB_POINT16 win_origin = {0, 0};
    if (!self || !CView_IsVisible_1010_1de8(self)) {
        return 0;
    }
    win = CView_GetWindow_1010_1d74(self);
    if (win && win->in_paint_52 && win->paint_hdc_48) {
        self->cached_hdc_28 = win->paint_hdc_48;
        self->owns_dc_35 = 0;
    } else if (win) {
        self->cached_hdc_28 = CWindow_GetDC_1010_55f0(win);
        self->owns_dc_35 = 1;
    } else {
        self->cached_hdc_28 = g_DAT_1060_01d2_screenDC;
        self->owns_dc_35 = 0;
    }
    if (!self->cached_hdc_28) {
        return 0;
    }
    CView_LocalToWindow_1010_45ea(self, &win_origin);
    sb_win16_SetWindowOrg(self->cached_hdc_28, (int16_t)(-win_origin.x), (int16_t)(-win_origin.y));
    self->dc_prepared_34 = 1;
    return 1;
}

/* `1010:4ac8` — `CView_RestoreDC_1010_4ac8` */
void CView_RestoreDC_1010_4ac8(SBView *self) {
    if (!self || !self->dc_prepared_34) {
        return;
    }
    if (self->cached_hdc_28) {
        sb_win16_SetWindowOrg(self->cached_hdc_28, 0, 0);
        if (self->owns_dc_35) {
            SBWindow *win = CView_GetWindow_1010_1d74(self);
            if (win) {
                CWindow_ReleaseDC_1010_5646(win, self->cached_hdc_28);
            }
        }
    }
    self->cached_hdc_28 = 0;
    self->owns_dc_35 = 0;
    self->dc_prepared_34 = 0;
}

/* `1010:4c18` — `CView_DrawSelf_1010_4c18` */
void CView_DrawSelf_1010_4c18(SBView *self, const SB_RECT16 *area_rc) {
    (void)area_rc;
    if (self && self->border_flags_24) {
        CView_DrawBorder_1010_4c50(self);
    }
}

/* `1010:4c50` — `CView_DrawBorder_1010_4c50` */
void CView_DrawBorder_1010_4c50(SBView *self) {
    SB_RECT16 bounds;
    if (!self || !self->cached_hdc_28) {
        return;
    }
    CView_GetBounds_1010_1c88(self, &bounds);
    CView_PenMoveTo_1010_4e8e(self, bounds.left, bounds.top);
    CView_PenLineTo_1010_4ec4(self, (int16_t)(bounds.right - 1), bounds.top);
    CView_PenLineTo_1010_4ec4(self, (int16_t)(bounds.right - 1), (int16_t)(bounds.bottom - 1));
    CView_PenLineTo_1010_4ec4(self, bounds.left, (int16_t)(bounds.bottom - 1));
    CView_PenLineTo_1010_4ec4(self, bounds.left, bounds.top);
}

/* `1010:4b30` — `CView_DrawAll_1010_4b30` */
void CView_DrawAll_1010_4b30(SBView *self, const SB_RECT16 *area_rc) {
    SB_RECT16 bounds;
    int16_t i;
    if (!self || !self->visible_1d) {
        return;
    }
    CView_GetBounds_1010_1c88(self, &bounds);
    if (!area_rc) {
        area_rc = &bounds;
    }
    if (CView_PrepareDC_1010_4a0a(self)) {
        CView_DrawSelf_1010_4c18(self, area_rc);
        CView_RestoreDC_1010_4ac8(self);
    }
    if (self->subviews_08) {
        for (i = 1; i <= self->subviews_08->count_04; ++i) {
            SBView *child = (SBView *)CObList_At_1008_c3ba(self->subviews_08, i);
            if (child && child->visible_1d) {
                CView_DrawAll_1010_4b30(child, NULL);
            }
        }
    }
}

/* `1010:4d5a` — `CView_InvalidateRect_1010_4d5a` */
void CView_InvalidateRect_1010_4d5a(SBView *self, const SB_RECT16 *local_rc) {
    SB_RECT16 bounds;
    SB_RECT16 wind_rc;
    SBWindow *win;
    if (!self) {
        return;
    }
    if (!local_rc) {
        CView_GetBounds_1010_1c88(self, &bounds);
        local_rc = &bounds;
    }
    CView_FrameToWindR_1010_4792(self, local_rc, &wind_rc);
    win = CView_GetWindow_1010_1d74(self);
    if (win) {
        CWindow_InvalidateWindRect_1010_5748(win, &wind_rc);
    }
}

/* `1010:4dd8` — `CView_ValidateRect_1010_4dd8` */
void CView_ValidateRect_1010_4dd8(SBView *self, const SB_RECT16 *local_rc) {
    SB_RECT16 bounds;
    SB_RECT16 wind_rc;
    SBWindow *win;
    if (!self) {
        return;
    }
    if (!local_rc) {
        CView_GetBounds_1010_1c88(self, &bounds);
        local_rc = &bounds;
    }
    CView_FrameToWindR_1010_4792(self, local_rc, &wind_rc);
    win = CView_GetWindow_1010_1d74(self);
    if (win) {
        CWindow_ValidateWindRect_1010_5792(win, &wind_rc);
    }
}

/* `1010:4e56` — `CView_InvalidateAll_1010_4e56` */
void CView_InvalidateAll_1010_4e56(SBView *self) {
    CView_InvalidateRect_1010_4d5a(self, NULL);
}

/* `1010:4e8e` — `CView_PenMoveTo_1010_4e8e` */
void CView_PenMoveTo_1010_4e8e(SBView *self, int16_t x, int16_t y) {
    if (!self) {
        return;
    }
    self->pen_x_30 = x;
    self->pen_y_32 = y;
}

/* `1010:4ec4` — `CView_PenLineTo_1010_4ec4` */
void CView_PenLineTo_1010_4ec4(SBView *self, int16_t x, int16_t y) {
    if (!self) {
        return;
    }
    self->pen_x_30 = x;
    self->pen_y_32 = y;
}

/* `1010:4f18` — `CView_FillRectColor_1010_4f18` */
void CView_FillRectColor_1010_4f18(SBView *self, const SB_RECT16 *rc, uint32_t color_ref) {
    if (!self || !rc) {
        return;
    }
    if (CView_PrepareDC_1010_4a0a(self)) {
        sb_win16_FillRectColor(self->cached_hdc_28, rc, (uint8_t)(color_ref & 0xFFu));
        CView_RestoreDC_1010_4ac8(self);
    }
}

/* `1010:4f78` — `CView_AdjustCursor_1010_4f78` */
void CView_AdjustCursor_1010_4f78(SBView *self, SB_POINT16 local_pt) {
    (void)self;
    (void)local_pt;
}

/* `1010:4fc0` — `CView_Noop_1010_4fc0` */
void CView_Noop_1010_4fc0(SBView *self) {
    (void)self;
}

/* `1008:31b8` — `CDesktop_Ctor_1008_31b8` */
SBDesktop *CDesktop_Ctor_1008_31b8(SBDesktop *self, SBEventTarget *next_handler) {
    if (!self) {
        self = (SBDesktop *)calloc(1, sizeof(SBDesktop));
    } else {
        memset(self, 0, sizeof(SBDesktop));
    }
    if (!self) {
        return NULL;
    }
    CView_Ctor_1010_15f0(&self->view_00, NULL, next_handler, 0, 0, 640, 480, 0);
    self->view_00.base_00.vtable_tag_00 = 0x10087d10u;
    self->floating_views_38 = CObList_New_1010_036c();
    self->top_window_3c = NULL;
    self->desktop_window_40 = NULL;
    g_DAT_1060_0810_desktop = self;
    return self;
}

/* `1008:31f0` — `CDesktop_Dtor_1008_31f0` */
void CDesktop_Dtor_1008_31f0(SBDesktop *self) {
    if (!self) {
        return;
    }
    if (self->floating_views_38) {
        CObList_Dtor_1008_c11a(self->floating_views_38);
        free(self->floating_views_38);
        self->floating_views_38 = NULL;
    }
    if (g_DAT_1060_0810_desktop == self) {
        g_DAT_1060_0810_desktop = NULL;
    }
    CView_Dtor_1010_16d4(&self->view_00);
}

/* `1008:3218` — `CDesktop_BringWindowToFront_1008_3218` */
void CDesktop_BringWindowToFront_1008_3218(SBDesktop *self, SBWindow *win) {
    if (!self || !win) {
        return;
    }
    self->top_window_3c = win;
    if (self->view_00.subviews_08) {
        CObList_MoveToBack_1008_c71a(self->view_00.subviews_08, win);
    }
}

/* `1008:d1d8` — `CDesktop_New_1008_d1d8` */
SBDesktop *CDesktop_New_1008_d1d8(SBEventTarget *next_handler) {
    return CDesktop_Ctor_1008_31b8(NULL, next_handler);
}

/* =========================================================================
 * 4. Window Wrapper (`CWindow_1010_5942`)
 * ========================================================================= */

/* `1010:2d16` — `Window_RegisterClasses_1010_2d16` */
void Window_RegisterClasses_1010_2d16(uint16_t hInstance) {
    uint8_t wc[32];
    if (!sb_win16_GetClassInfo(hInstance, "c window", wc)) {
        sb_win16_RegisterClass("c window");
    }
    if (!sb_win16_GetClassInfo(hInstance, "p window", wc)) {
        sb_win16_RegisterClass("p window");
    }
}

/* `1010:2d22` — `Window_GetMainScreenDC_1010_2d22` */
SB_HDC Window_GetMainScreenDC_1010_2d22(void) {
    if (!g_DAT_1060_01d2_screenDC) {
        g_DAT_1060_01d2_screenDC = sb_win16_GetDC(0);
    }
    return g_DAT_1060_01d2_screenDC;
}

/* `1010:5332` — `CWindow_CreateNativeWindow_1010_5332` */
void CWindow_CreateNativeWindow_1010_5332(SBWindow *self, const char *title, uint8_t show_now) {
    const char *cls_name;
    if (!self) {
        return;
    }
    Window_RegisterClasses_1010_2d16(g_DAT_1060_1d58_hInstance);
    cls_name = self->is_floating_38 ? "p window" : "c window";
    self->hwnd_42 = sb_win16_CreateWindow(cls_name,
                                          title ? title : "Spelling Blizzard",
                                          self->win_style_44,
                                          self->view_00.loc_x_10,
                                          self->view_00.loc_y_12,
                                          self->view_00.width_14,
                                          self->view_00.height_16,
                                          0,
                                          0,
                                          g_DAT_1060_1d58_hInstance,
                                          self);
    sb_win16_SetWindowLongPtr(self->hwnd_42, 0, self);
    if (show_now) {
        sb_win16_ShowWindow(self->hwnd_42, 1);
    }
    if (!g_DAT_1060_01d2_screenDC) {
        g_DAT_1060_01d2_screenDC = sb_win16_GetDC(self->hwnd_42);
    }
}

/* `1010:51b4` — `CWindow_CtorBounds_1010_51b4` */
SBWindow *CWindow_CtorBounds_1010_51b4(SBWindow *self,
                                       const SB_RECT16 *bounds,
                                       const char *title,
                                       uint8_t is_visible,
                                       uint16_t proc_id,
                                       uint8_t has_close_box,
                                       uint8_t is_floating,
                                       SBView *superview,
                                       SBEventTarget *next_handler) {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 640;
    int16_t h = 480;

    if (!self) {
        self = (SBWindow *)calloc(1, sizeof(SBWindow));
    } else {
        memset(self, 0, sizeof(SBWindow));
    }
    if (!self) {
        return NULL;
    }
    if (bounds) {
        x = bounds->left;
        y = bounds->top;
        w = (int16_t)(bounds->right - bounds->left);
        h = (int16_t)(bounds->bottom - bounds->top);
    }
    if (!superview && g_DAT_1060_0810_desktop) {
        superview = &g_DAT_1060_0810_desktop->view_00;
    }
    CView_Ctor_1010_15f0(&self->view_00, superview, next_handler, x, y, w, h, 0);
    self->view_00.base_00.vtable_tag_00 = 0x10105942u;
    self->view_00.visible_1d = is_visible;
    self->is_floating_38 = is_floating;
    self->is_active_3a = 1;
    self->is_closable_3b = has_close_box;
    self->proc_id_3c = proc_id;
    self->has_close_box_3e = has_close_box;
    CWindow_CreateNativeWindow_1010_5332(self, title, is_visible);
    if (g_DAT_1060_0810_desktop) {
        CDesktop_BringWindowToFront_1008_3218(g_DAT_1060_0810_desktop, self);
    }
    return self;
}

/* `1010:50ce` — `CWindow_Ctor_1010_50ce`
 *
 * Loads a Macintosh `'WIND'` (`0x03f0`) resource:
 *   +0x00: int16_tbe top
 *   +0x02: int16_tbe left
 *   +0x04: int16_tbe bottom
 *   +0x06: int16_tbe right
 *   +0x08: uint16_tbe procID
 *   +0x0a: uint8_t   visible
 *   +0x0c: uint8_t   goAwayFlag
 *   +0x0e: uint32_tbe refCon
 *   +0x12: Pascal string title
 */
SBWindow *CWindow_Ctor_1010_50ce(SBWindow *self,
                                 uint16_t wind_res_id,
                                 uint8_t is_floating,
                                 SBView *superview,
                                 SBEventTarget *next_handler) {
    SBResourceHandle *hWind;
    SB_RECT16 bounds = {0, 0, 640, 480};
    uint16_t proc_id = 0;
    uint8_t vis = 1;
    uint8_t close_box = 1;
    char title[64];

    strcpy(title, "Spelling Blizzard");
    hWind = ResMgr_GetResource_1010_10f2(0x03f0u, "WIND", wind_res_id);
    if (hWind) {
        const uint8_t *p;
        Handle_Lock_1010_00e8(hWind);
        p = (const uint8_t *)hWind->data_00;
        if (p && hWind->size_08 >= 18) {
            int16_t top = read_be16s(p + 0);
            int16_t left = read_be16s(p + 2);
            int16_t bottom = read_be16s(p + 4);
            int16_t right = read_be16s(p + 6);
            proc_id = read_be16(p + 8);
            vis = (uint8_t)(p[10] != 0 ? 1 : 0);
            close_box = (uint8_t)(p[12] != 0 ? 1 : 0);
            if (right > left && bottom > top) {
                Rect_Set_1010_0fb6(&bounds, left, top, right, bottom);
            }
            if (hWind->size_08 > 18) {
                uint8_t tlen = p[18];
                if (tlen > 0 && (uint32_t)(19 + tlen) <= hWind->size_08) {
                    if (tlen >= sizeof(title)) {
                        tlen = (uint8_t)(sizeof(title) - 1);
                    }
                    memcpy(title, p + 19, tlen);
                    title[tlen] = '\0';
                }
            }
        }
        Handle_Unlock_1010_0122(hWind);
    }
    self = CWindow_CtorBounds_1010_51b4(self,
                                        &bounds,
                                        title,
                                        vis,
                                        proc_id,
                                        close_box,
                                        is_floating,
                                        superview,
                                        next_handler);
    if (self) {
        self->view_00.pane_id_20 = (int32_t)wind_res_id;
    }
    return self;
}

/* `1010:528e` — `CWindow_Dtor_1010_528e` */
void CWindow_Dtor_1010_528e(SBWindow *self) {
    if (!self) {
        return;
    }
    if (self->hwnd_42) {
        sb_win16_SetWindowLongPtr(self->hwnd_42, 0, NULL);
        sb_win16_DestroyWindow(self->hwnd_42);
        self->hwnd_42 = 0;
    }
    if (g_DAT_1060_0810_desktop && g_DAT_1060_0810_desktop->top_window_3c == self) {
        g_DAT_1060_0810_desktop->top_window_3c = NULL;
    }
    CView_Dtor_1010_16d4(&self->view_00);
}

/* `1010:52f4` — `CWindow_Dispose_1010_52f4` */
void CWindow_Dispose_1010_52f4(SBWindow *self) {
    CWindow_Dtor_1010_528e(self);
}

/* `1010:5490` — `CWindow_Select_1010_5490` */
void CWindow_Select_1010_5490(SBWindow *self) {
    if (!self) {
        return;
    }
    if (g_DAT_1060_0810_desktop) {
        CDesktop_BringWindowToFront_1008_3218(g_DAT_1060_0810_desktop, self);
    }
    CWindow_Activate_1010_5540(self, 1);
}

/* `1010:54dc` — `CWindow_Show_1010_54dc` */
void CWindow_Show_1010_54dc(SBWindow *self, uint8_t visible, uint8_t redraw) {
    if (!self) {
        return;
    }
    CView_Show_1010_1ae4(&self->view_00, visible, redraw);
    if (self->hwnd_42) {
        sb_win16_ShowWindow(self->hwnd_42, visible ? 1 : 0);
    }
}

/* `1010:5540` — `CWindow_Activate_1010_5540` */
void CWindow_Activate_1010_5540(SBWindow *self, uint8_t active) {
    if (!self) {
        return;
    }
    self->is_active_3a = (uint8_t)(active ? 1 : 0);
    CView_Activate_1010_1b58(&self->view_00, active);
}

/* `1010:55a8` — `CWindow_Close_1010_55a8` */
void CWindow_Close_1010_55a8(SBWindow *self) {
    if (!self) {
        return;
    }
    CWindow_Show_1010_54dc(self, 0, 0);
}

/* `1010:55f0` — `CWindow_GetDC_1010_55f0` */
SB_HDC CWindow_GetDC_1010_55f0(SBWindow *self) {
    if (!self) {
        return g_DAT_1060_01d2_screenDC;
    }
    if (self->in_paint_52 && self->paint_hdc_48) {
        return self->paint_hdc_48;
    }
    return sb_win16_GetDC(self->hwnd_42);
}

/* `1010:5646` — `CWindow_ReleaseDC_1010_5646` */
void CWindow_ReleaseDC_1010_5646(SBWindow *self, SB_HDC hdc) {
    if (!self || !hdc) {
        return;
    }
    if (self->in_paint_52 && hdc == self->paint_hdc_48) {
        return;
    }
    sb_win16_ReleaseDC(self->hwnd_42, hdc);
}

/* `1010:568a` — `CWindow_Update_1010_568a` */
void CWindow_Update_1010_568a(SBWindow *self) {
    if (!self) {
        return;
    }
    CWindow_OnPaint_1010_5b60(self);
}

/* `1010:5748` — `CWindow_InvalidateWindRect_1010_5748` */
void CWindow_InvalidateWindRect_1010_5748(SBWindow *self, const SB_RECT16 *wind_rc) {
    if (!self) {
        return;
    }
    if (wind_rc) {
        Rect_Union_1010_0fd0(&self->update_rc_4a, &self->update_rc_4a, wind_rc);
    } else {
        CView_GetBounds_1010_1c88(&self->view_00, &self->update_rc_4a);
    }
    if (self->hwnd_42) {
        sb_win16_InvalidateRect(self->hwnd_42, wind_rc, 0);
    }
}

/* `1010:5792` — `CWindow_ValidateWindRect_1010_5792` */
void CWindow_ValidateWindRect_1010_5792(SBWindow *self, const SB_RECT16 *wind_rc) {
    if (!self) {
        return;
    }
    if (self->hwnd_42) {
        sb_win16_ValidateRect(self->hwnd_42, wind_rc);
    }
}

/* `1010:57dc` — `CWindow_MoveWindow_1010_57dc` */
void CWindow_MoveWindow_1010_57dc(SBWindow *self, int16_t x, int16_t y) {
    if (!self) {
        return;
    }
    CView_MoveTo_1010_1cbe(&self->view_00, x, y, 1);
}

/* `1010:5834` — `CWindow_SizeWindow_1010_5834` */
void CWindow_SizeWindow_1010_5834(SBWindow *self, int16_t w, int16_t h) {
    if (!self) {
        return;
    }
    CView_Resize_1010_1d12(&self->view_00, w, h, 1);
}

/* `1010:588c` — `CWindow_GetGlobalBounds_1010_588c` */
void CWindow_GetGlobalBounds_1010_588c(const SBWindow *self, SB_RECT16 *out_rc) {
    if (!self || !out_rc) {
        return;
    }
    CView_GetFrame_1010_1c52(&self->view_00, out_rc);
}

/* `1010:59c2` — `CWindow_FromHwnd_1010_59c2` */
SBWindow *CWindow_FromHwnd_1010_59c2(SB_HWND hwnd) {
    if (!hwnd) {
        return NULL;
    }
    return (SBWindow *)sb_win16_GetWindowLongPtr(hwnd, 0);
}

/* `1010:5b60` — `CWindow_OnPaint_1010_5b60` */
void CWindow_OnPaint_1010_5b60(SBWindow *self) {
    SB_RECT16 bounds;
    if (!self) {
        return;
    }
    self->paint_hdc_48 = sb_win16_GetDC(self->hwnd_42);
    self->in_paint_52 = 1;
    CView_GetBounds_1010_1c88(&self->view_00, &bounds);
    CView_DrawAll_1010_4b30(&self->view_00, &bounds);
    self->in_paint_52 = 0;
    self->paint_hdc_48 = 0;
    Rect_Set_1010_0fb6(&self->update_rc_4a, 0, 0, 0, 0);
    sb_win16_ValidateRect(self->hwnd_42, NULL);
}

/* `1010:5c24` — `CWindow_OnActivate_1010_5c24` */
void CWindow_OnActivate_1010_5c24(SBWindow *self, uint16_t active_flag) {
    if (!self) {
        return;
    }
    CWindow_Activate_1010_5540(self, (uint8_t)(active_flag != 0 ? 1 : 0));
}

/* `1010:5c78` — `CWindow_OnMove_1010_5c78` */
void CWindow_OnMove_1010_5c78(SBWindow *self, int16_t new_x, int16_t new_y) {
    if (!self) {
        return;
    }
    self->view_00.loc_x_10 = new_x;
    self->view_00.loc_y_12 = new_y;
}

/* `1010:5cc4` — `CWindow_OnSize_1010_5cc4` */
void CWindow_OnSize_1010_5cc4(SBWindow *self, int16_t new_w, int16_t new_h) {
    if (!self) {
        return;
    }
    self->view_00.width_14 = new_w;
    self->view_00.height_16 = new_h;
}

/* `1010:5d20` — `CWindow_OnClose_1010_5d20` */
int CWindow_OnClose_1010_5d20(SBWindow *self) {
    if (!self) {
        return 0;
    }
    CWindow_Close_1010_55a8(self);
    if (g_DAT_1060_07c4_app) {
        CApplication_RequestQuit_1008_7bf2(g_DAT_1060_07c4_app);
    }
    return 1;
}

/* `1010:5d7c` — `CWindow_GetHwnd_1010_5d7c` */
SB_HWND CWindow_GetHwnd_1010_5d7c(const SBWindow *self) {
    return self ? self->hwnd_42 : 0;
}

/* `1010:5a18` — `CWindow_WndProc_1010_5a18` */
uint32_t CWindow_WndProc_1010_5a18(SB_HWND hwnd, uint16_t message, uint16_t wParam, uint32_t lParam) {
    SBWindow *win = CWindow_FromHwnd_1010_59c2(hwnd);
    if (!win) {
        return 0;
    }
    switch (message) {
        case SB_WM_PAINT:
            CWindow_OnPaint_1010_5b60(win);
            return 0;
        case SB_WM_ACTIVATE:
            CWindow_OnActivate_1010_5c24(win, wParam);
            return 0;
        case SB_WM_MOVE:
            CWindow_OnMove_1010_5c78(win, (int16_t)(lParam & 0xFFFFu), (int16_t)((lParam >> 16) & 0xFFFFu));
            return 0;
        case SB_WM_SIZE:
            CWindow_OnSize_1010_5cc4(win, (int16_t)(lParam & 0xFFFFu), (int16_t)((lParam >> 16) & 0xFFFFu));
            return 0;
        case SB_WM_CLOSE:
            CWindow_OnClose_1010_5d20(win);
            return 0;
        case SB_WM_ACTIVATEAPP:
            if (g_DAT_1060_07c4_app) {
                CApplication_OnActivateApp_1008_7940(g_DAT_1060_07c4_app, (uint8_t)(wParam != 0 ? 1 : 0));
            }
            return 0;
        default:
            return 0;
    }
}

/* =========================================================================
 * 5. Application Lifecycle (`CApplication_1008_2e32`)
 * ========================================================================= */

/* `1008:018c` — `CApplication_Ctor_1008_018c` */
SBApplication *CApplication_Ctor_1008_018c(SBApplication *self,
                                           int16_t extra_masters,
                                           uint32_t rain_reserve,
                                           uint32_t total_reserve) {
    (void)extra_masters;
    (void)rain_reserve;
    (void)total_reserve;
    if (!self) {
        self = (SBApplication *)calloc(1, sizeof(SBApplication));
    } else {
        memset(self, 0, sizeof(SBApplication));
    }
    if (!self) {
        return NULL;
    }
    CEventDispatcher_Init_1008_7c02(&self->dispatcher_00);
    self->dispatcher_00.base_00.vtable_tag_00 = 0x10082e32u;
    self->doc_list_62 = CObList_New_1010_036c();
    self->quit_requested_66 = 0;
    self->low_memory_67 = 0;
    self->is_active_app_68 = 1;
    self->hInstance_6c = g_DAT_1060_1d58_hInstance;
    self->cmd_show_6a = g_DAT_1060_1d56_nCmdShow;
    self->main_wind_res_id_70 = 0x0419u; /* 1049 */
    self->clut_res_id_72 = 0x0419u;      /* 1049 */
    g_DAT_1060_07c4_app = self;
    return self;
}

/* `1008:0240` — `CApplication_Dtor_1008_0240` */
void CApplication_Dtor_1008_0240(SBApplication *self) {
    if (!self) {
        return;
    }
    if (self->doc_list_62) {
        CObList_Dtor_1008_c11a(self->doc_list_62);
        free(self->doc_list_62);
        self->doc_list_62 = NULL;
    }
    if (g_DAT_1060_07c4_app == self) {
        g_DAT_1060_07c4_app = NULL;
    }
    CEventDispatcherBase_Dtor_1010_3ae8(&self->dispatcher_00);
}

/* `1008:02a8` — `CApplication_InitToolbox_1008_02a8` */
void CApplication_InitToolbox_1008_02a8(SBApplication *self) {
    if (!self) {
        return;
    }
    CEventDispatcherBase_Init_1010_3b2c(&self->dispatcher_00);
    Window_RegisterClasses_1010_2d16(self->hInstance_6c);
}

/* `1008:0310` — `CApplication_CreateDesktop_1008_0310` */
void CApplication_CreateDesktop_1008_0310(SBApplication *self) {
    if (!self) {
        return;
    }
    if (!g_DAT_1060_0810_desktop) {
        g_DAT_1060_0810_desktop = CDesktop_New_1008_d1d8(&self->dispatcher_00.base_00);
    }
}

/* `1008:0368` — `CApplication_HandleCommand_1008_0368` */
int CApplication_HandleCommand_1008_0368(SBApplication *self, int32_t cmd) {
    if (!self) {
        return 0;
    }
    if (cmd == 1 || cmd == 0x71756974) { /* 'quit' */
        CApplication_Quit_1008_0412(self);
        return 1;
    }
    return 0;
}

/* `1008:0458` — `CApplication_CanQuit_1008_0458` */
int CApplication_CanQuit_1008_0458(SBApplication *self) {
    (void)self;
    return 1;
}

/* `1008:0412` — `CApplication_Quit_1008_0412` */
void CApplication_Quit_1008_0412(SBApplication *self) {
    if (!self) {
        return;
    }
    if (CApplication_CanQuit_1008_0458(self)) {
        self->quit_requested_66 = 1;
    }
}

/* `1008:7610` — `CApplication_CreateMainWindow_1008_7610` */
void CApplication_CreateMainWindow_1008_7610(SBApplication *self) {
    if (!self) {
        return;
    }
    if (!self->main_window_74) {
        self->main_window_74 = CWindow_Ctor_1010_50ce(NULL,
                                                      self->main_wind_res_id_70,
                                                      0,
                                                      g_DAT_1060_0810_desktop ? &g_DAT_1060_0810_desktop->view_00 : NULL,
                                                      &self->dispatcher_00.base_00);
        if (g_DAT_1060_0810_desktop) {
            g_DAT_1060_0810_desktop->desktop_window_40 = self->main_window_74;
        }
    }
}

/* `1008:7720` — `CApplication_LoadInitialPalette_1008_7720` */
void CApplication_LoadInitialPalette_1008_7720(SBApplication *self, uint16_t clut_res_id) {
    if (!self) {
        return;
    }
    if (!g_DAT_1060_0198_paletteMgr) {
        SBPaletteManager *pm = (SBPaletteManager *)NewObject_1008_ef2c(sizeof(SBPaletteManager));
        g_DAT_1060_0198_paletteMgr = CPaletteManager_Init_1008_326e(pm);
    }
    if (g_DAT_1060_0198_paletteMgr) {
        CPaletteManager_LoadClutResource_1008_36b2(g_DAT_1060_0198_paletteMgr, clut_res_id);
        CPaletteManager_ApplyTargetRange_1008_3596(g_DAT_1060_0198_paletteMgr, 236, 1);
    }
}

/* `1008:7790` — `CApplication_CreateOffscreenBuffer_1008_7790` */
void CApplication_CreateOffscreenBuffer_1008_7790(SBApplication *self) {
    SB_RECT16 screen_rc = {0, 0, 640, 480};
    if (!self) {
        return;
    }
    if (!self->offscreen_78) {
        self->offscreen_78 = (SBPictSurface *)NewObject_1008_ef2c(sizeof(SBPictSurface));
        if (self->offscreen_78) {
            CPictSurface_InitBlank_1008_5646(self->offscreen_78, 1, 8, &screen_rc);
        }
    }
}

/* `1008:7820` — `CApplication_PreloadCoreResources_1008_7820` */
void CApplication_PreloadCoreResources_1008_7820(SBApplication *self) {
    (void)self;
    if (g_DAT_1060_0a14_resMgr) {
        SBResourceBatchLoader *batch =
            (SBResourceBatchLoader *)NewObject_1008_ef2c(sizeof(SBResourceBatchLoader));
        if (batch) {
            CResourceList_Init_1018_9140(batch, 0x01f5u); /* RsLt #501 */
            CResourceList_PreloadAll_1018_9480(batch);
            CResourceList_Dtor_1018_9176(batch);
        }
    }
}

/* `1008:78b0` — `CApplication_StartGameController_1008_78b0` */
void CApplication_StartGameController_1008_78b0(SBApplication *self) {
    if (!self) {
        return;
    }
    g_DAT_1060_07f4_appReady = 1;
}

/* `1008:72b6` — `CApplication_InitGameSubsystems_1008_72b6` */
void CApplication_InitGameSubsystems_1008_72b6(SBApplication *self) {
    if (!self) {
        return;
    }
    CApplication_InitToolbox_1008_02a8(self);
    if (!g_DAT_1060_0a2e_idleList) {
        g_DAT_1060_0a2e_idleList = CObList_New_1010_036c();
    }
    if (!g_DAT_1060_07c8_soundMgr) {
        SBSoundManager *sm = (SBSoundManager *)NewObject_1008_ef2c(sizeof(SBSoundManager));
        g_DAT_1060_07c8_soundMgr = CSoundManager_Init_1008_21ec(sm);
    }
    CApplication_CreateDesktop_1008_0310(self);
    CApplication_CreateMainWindow_1008_7610(self);
    CApplication_LoadInitialPalette_1008_7720(self, self->clut_res_id_72);
    CApplication_CreateOffscreenBuffer_1008_7790(self);
    CApplication_PreloadCoreResources_1008_7820(self);
    CApplication_StartGameController_1008_78b0(self);
}

/* `1008:7540` — `CApplication_CleanupGameSubsystems_1008_7540` */
void CApplication_CleanupGameSubsystems_1008_7540(SBApplication *self) {
    if (!self) {
        return;
    }
    if (self->offscreen_78) {
        CPictSurface_Dtor_1008_56a8(self->offscreen_78);
        self->offscreen_78 = NULL;
    }
    if (self->main_window_74) {
        CWindow_Dtor_1010_528e(self->main_window_74);
        free(self->main_window_74);
        self->main_window_74 = NULL;
    }
    if (g_DAT_1060_0810_desktop) {
        CDesktop_Dtor_1008_31f0(g_DAT_1060_0810_desktop);
        free(g_DAT_1060_0810_desktop);
        g_DAT_1060_0810_desktop = NULL;
    }
    if (g_DAT_1060_0198_paletteMgr) {
        CPaletteManager_Dtor_1008_3464(g_DAT_1060_0198_paletteMgr);
        g_DAT_1060_0198_paletteMgr = NULL;
    }
    if (g_DAT_1060_07c8_soundMgr) {
        CSoundManager_Dtor_1008_225a(g_DAT_1060_07c8_soundMgr);
        g_DAT_1060_07c8_soundMgr = NULL;
    }
    if (g_DAT_1060_0a2e_idleList) {
        CObList_Dtor_1008_c11a(g_DAT_1060_0a2e_idleList);
        free(g_DAT_1060_0a2e_idleList);
        g_DAT_1060_0a2e_idleList = NULL;
    }
    g_DAT_1060_07f4_appReady = 0;
}

/* `1008:7940` — `CApplication_OnActivateApp_1008_7940` */
void CApplication_OnActivateApp_1008_7940(SBApplication *self, uint8_t active) {
    if (!self) {
        return;
    }
    self->is_active_app_68 = active;
    if (g_DAT_1060_0198_paletteMgr) {
        if (active) {
            CPaletteManager_RestoreAndRealize_1008_3950(g_DAT_1060_0198_paletteMgr);
        } else {
            CPaletteManager_SaveAndRealize_1008_382e(g_DAT_1060_0198_paletteMgr);
        }
    }
}

/* `1008:7a80` — `CApplication_DispatchTimers_1008_7a80` */
void CApplication_DispatchTimers_1008_7a80(SBApplication *self) {
    (void)self;
    if (g_DAT_1060_0a1e_timer60Hz) {
        CTimer60Hz_GetTicks_1010_44ec(g_DAT_1060_0a1e_timer60Hz);
    }
}

/* `1008:79ec` — `CApplication_PumpIdleAndAudio_1008_79ec` */
void CApplication_PumpIdleAndAudio_1008_79ec(SBApplication *self) {
    if (!self) {
        return;
    }
    CApplication_DispatchTimers_1008_7a80(self);
    if (g_DAT_1060_07c8_soundMgr) {
        CSoundManager_IdleTimerCheck_1008_24ec(g_DAT_1060_07c8_soundMgr);
    }
    CEventDispatcherBase_DoIdle_1010_4120(&self->dispatcher_00);
}

/* `1008:06c8` — `CApplication_ProcessSingleStep_1008_06c8` */
void CApplication_ProcessSingleStep_1008_06c8(SBApplication *self) {
    if (!self) {
        return;
    }
    CEventDispatcherBase_ProcessOneEvent_1010_3bb2(&self->dispatcher_00);
    CApplication_PumpIdleAndAudio_1008_79ec(self);
}

/* `1008:0694` — `CApplication_RunMainLoop_1008_0694` */
void CApplication_RunMainLoop_1008_0694(SBApplication *self) {
    if (!self) {
        return;
    }
    while (!self->quit_requested_66) {
        CApplication_ProcessSingleStep_1008_06c8(self);
    }
}

/* `1008:0700` — `CApplication_Shutdown_1008_0700` */
void CApplication_Shutdown_1008_0700(SBApplication *self) {
    if (!self) {
        return;
    }
    CApplication_CleanupGameSubsystems_1008_7540(self);
    CApplication_Dtor_1008_0240(self);
}

/* `1008:7b20` — `CApplication_GetOffscreen_1008_7b20` */
SBPictSurface *CApplication_GetOffscreen_1008_7b20(const SBApplication *self) {
    return self ? self->offscreen_78 : NULL;
}

/* `1008:7b50` — `CApplication_GetMainWindow_1008_7b50` */
SBWindow *CApplication_GetMainWindow_1008_7b50(const SBApplication *self) {
    return self ? self->main_window_74 : NULL;
}

/* `1008:7b80` — `CApplication_PresentOffscreenRect_1008_7b80` */
void CApplication_PresentOffscreenRect_1008_7b80(SBApplication *self, const SB_RECT16 *rc) {
    SB_RECT16 full_rc = {0, 0, 640, 480};
    if (!self || !self->offscreen_78) {
        return;
    }
    if (!rc) {
        rc = &full_rc;
    }
    CPictSurface_CopyToScreen_1008_5caa(self->offscreen_78, rc, rc);
}

/* `1008:7bf2` — `CApplication_RequestQuit_1008_7bf2` */
void CApplication_RequestQuit_1008_7bf2(SBApplication *self) {
    if (self) {
        self->quit_requested_66 = 1;
    }
}

/* `1008:2bf4` — `Application_WinMainEntry_1008_2bf4`
 *
 * Full startup sequence reconstructed from `1008:2bf4`:
 *   1. Check system requirements (`SysReq_CheckWaveOutAndColor_1008_6a28`)
 *   2. Initialize `CResourceManager_1008_2da6` (`DAT_1060_0a14`)
 *   3. Initialize `CTimer60Hz_1010_456a` (`DAT_1060_0a1e`)
 *   4. Construct `CApplication_1008_2e32` (`DAT_1060_07c4`)
 *   5. Call slot `+0x98` (`CApplication_InitGameSubsystems_1008_72b6`)
 *   6. Run main loop (`CApplication_RunMainLoop_1008_0694`)
 *   7. Call slot `+0x58` (`CApplication_Shutdown_1008_0700`)
 */
int Application_WinMainEntry_1008_2bf4(uint16_t hInstance,
                                       uint16_t hPrevInstance,
                                       const char *lpCmdLine,
                                       uint16_t nCmdShow,
                                       uint16_t max_loop_steps) {
    SBApplication *app;
    uint16_t step = 0;
    (void)lpCmdLine;

    g_DAT_1060_1d58_hInstance = hInstance;
    g_DAT_1060_1d56_nCmdShow = nCmdShow;

    if (!System_CheckRequirements_1008_6a28()) {
        return -1;
    }
    if (!g_DAT_1060_0a14_resMgr) {
        sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL");
        ResMgr_LoadCdResLibrary_1008_2bbc();
        CResourceManager_CreateGlobal_1008_2bf4_part();
    }
    if (!g_DAT_1060_0a1e_timer60Hz) {
        SBTimer60Hz *tm = (SBTimer60Hz *)NewObject_1008_ef2c(sizeof(SBTimer60Hz));
        g_DAT_1060_0a1e_timer60Hz = CTimer60Hz_Ctor_1010_446a(tm);
    }

    app = CApplication_Ctor_1008_018c(NULL, 4, 0x8000u, 0x9400u);
    if (!app) {
        return -1;
    }
    app->hPrevInstance_6e = hPrevInstance;

    CApplication_InitGameSubsystems_1008_72b6(app);

    while (!app->quit_requested_66) {
        CApplication_ProcessSingleStep_1008_06c8(app);
        step++;
        if (max_loop_steps > 0 && step >= max_loop_steps) {
            CApplication_RequestQuit_1008_7bf2(app);
        }
    }

    CApplication_Shutdown_1008_0700(app);
    free(app);
    return 0;
}

#ifndef SB_RECOMP_LOADER_H
#define SB_RECOMP_LOADER_H

#include <stddef.h>
#include <stdint.h>

#define MAX_NE_SEGS 64

typedef struct NESeg {
    uint32_t file_off;
    uint32_t len;
    uint16_t flags;
    uint32_t minalloc;
    uint16_t sel;
} NESeg;

typedef struct NEModule {
    char path[512];
    char name[16];
    uint8_t *raw;
    size_t raw_len;
    uint32_t ne_off;
    uint16_t autodata, heap, stack, ip, cs, sp, ss, nseg, nmod, align;
    uint32_t segtab, rsrctab, restab, modref, impnames;
    NESeg seg[MAX_NE_SEGS];
    uint16_t hmodule;
    uint16_t ncmdshow;
    uint16_t dgroup_size;
} NEModule;

extern NEModule g_exe;
extern NEModule g_dll;
extern uint16_t g_psp_sel;
extern uint16_t g_env_sel;

int loader_load_exe(const char *path);
int loader_load_dll(const char *path, uint16_t hmodule);
void loader_setup_task(const char *cmdline, uint16_t ncmdshow);

#endif

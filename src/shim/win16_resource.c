#include "win16_resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_MAX_NE_MODULES 4
#define SB_MAX_NE_RESOURCES 2048

typedef struct SBNeResourceEntry {
    SB_HMODULE hModule;
    uint16_t type_id;     /* Normalized 15-bit integer ID (e.g. 0x03ee for 0x83ee) */
    uint16_t res_id;      /* Normalized 15-bit integer ID (e.g. 1001 for 0x83e9) */
    uint16_t flags;
    uint32_t file_offset;
    uint32_t byte_length;
    uint8_t is_loaded;
} SBNeResourceEntry;

typedef struct SBNeModule {
    SB_HMODULE handle;
    char name[64];
    uint8_t *file_bytes;
    size_t file_size;
    uint16_t first_res_index;
    uint16_t res_count;
    uint8_t in_use;
} SBNeModule;

static SBNeModule g_modules[SB_MAX_NE_MODULES];
static SBNeResourceEntry g_resources[SB_MAX_NE_RESOURCES];
static uint16_t g_total_resources = 0;
static char g_cdres_default_path[512] = "original/SB_CDRES.DLL";

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int parse_ne_module_resources(SBNeModule *mod)
{
    const uint8_t *data = mod->file_bytes;
    size_t size = mod->file_size;
    uint16_t ne_off;
    uint16_t restab_rel;
    size_t restab;
    uint16_t align_shift;
    uint32_t align_factor;
    size_t p;

    if (size < 0x40 || data[0] != 'M' || data[1] != 'Z') {
        return -1;
    }
    ne_off = read_u16_le(data + 0x3C);
    if ((size_t)ne_off + 0x40 > size || data[ne_off] != 'N' || data[ne_off + 1] != 'E') {
        return -1;
    }
    restab_rel = read_u16_le(data + ne_off + 0x24);
    restab = (size_t)ne_off + restab_rel;
    if (restab + 4 > size) {
        return -1;
    }

    align_shift = read_u16_le(data + restab);
    align_factor = 1u << align_shift;
    p = restab + 2;

    mod->first_res_index = g_total_resources;
    mod->res_count = 0;

    while (p + 2 <= size) {
        uint16_t raw_type_id = read_u16_le(data + p);
        uint16_t count;
        uint16_t norm_type_id;
        uint16_t i;

        if (raw_type_id == 0) {
            break;
        }
        if (p + 8 > size) {
            return -1;
        }
        count = read_u16_le(data + p + 2);
        norm_type_id = (raw_type_id & 0x8000u) ? (raw_type_id & 0x7FFFu) : raw_type_id;
        p += 8;

        for (i = 0; i < count; ++i) {
            uint16_t rn_off;
            uint16_t rn_len;
            uint16_t rn_flags;
            uint16_t rn_id;
            uint32_t file_off;
            uint32_t byte_len;
            SBNeResourceEntry *entry;

            if (p + 12 > size || g_total_resources >= SB_MAX_NE_RESOURCES) {
                return -1;
            }
            rn_off = read_u16_le(data + p);
            rn_len = read_u16_le(data + p + 2);
            rn_flags = read_u16_le(data + p + 4);
            rn_id = read_u16_le(data + p + 6);
            p += 12;

            file_off = (uint32_t)rn_off * align_factor;
            byte_len = (uint32_t)rn_len * align_factor;
            if ((size_t)file_off + byte_len > size) {
                return -1;
            }

            entry = &g_resources[g_total_resources++];
            entry->hModule = mod->handle;
            entry->type_id = norm_type_id;
            entry->res_id = (rn_id & 0x8000u) ? (rn_id & 0x7FFFu) : rn_id;
            entry->flags = rn_flags;
            entry->file_offset = file_off;
            entry->byte_length = byte_len;
            entry->is_loaded = 0;
            mod->res_count++;
        }
    }
    return 0;
}

static int load_ne_file_into_slot(uint16_t slot, SB_HMODULE handle, const char *name, const char *path)
{
    FILE *fp;
    long len;
    uint8_t *buf;
    SBNeModule *mod = &g_modules[slot];

    if (mod->in_use && mod->file_bytes != NULL) {
        return 0;
    }

    fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    len = ftell(fp);
    if (len <= 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    buf = (uint8_t *)malloc((size_t)len);
    if (!buf) {
        fclose(fp);
        return -1;
    }
    if (fread(buf, 1, (size_t)len, fp) != (size_t)len) {
        free(buf);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    memset(mod, 0, sizeof(*mod));
    mod->handle = handle;
    strncpy(mod->name, name, sizeof(mod->name) - 1);
    mod->file_bytes = buf;
    mod->file_size = (size_t)len;
    mod->in_use = 1;

    if (parse_ne_module_resources(mod) != 0) {
        free(buf);
        memset(mod, 0, sizeof(*mod));
        return -1;
    }
    return 0;
}

int sb_win16_resource_shim_init(const char *blizzard_exe_path,
                                const char *sb_cdres_dll_path)
{
    const char *exe_path = blizzard_exe_path ? blizzard_exe_path : "original/BLIZZARD.EXE";
    const char *dll_path = sb_cdres_dll_path ? sb_cdres_dll_path : "original/SB_CDRES.DLL";

    sb_win16_resource_shim_shutdown();

    strncpy(g_cdres_default_path, dll_path, sizeof(g_cdres_default_path) - 1);
    g_cdres_default_path[sizeof(g_cdres_default_path) - 1] = '\0';

    if (load_ne_file_into_slot(0, SB_HMODULE_BLIZZARD_EXE, "BLIZZARD.EXE", exe_path) != 0) {
        return -1;
    }
    return 0;
}

void sb_win16_resource_shim_shutdown(void)
{
    size_t i;
    for (i = 0; i < SB_MAX_NE_MODULES; ++i) {
        if (g_modules[i].file_bytes != NULL) {
            free(g_modules[i].file_bytes);
        }
        memset(&g_modules[i], 0, sizeof(g_modules[i]));
    }
    memset(g_resources, 0, sizeof(g_resources));
    g_total_resources = 0;
}

SB_HMODULE sb_win16_LoadLibrary(const char *lib_file_name)
{
    if (!lib_file_name) {
        return 0;
    }
    if (strstr(lib_file_name, "SB_CDRES") != NULL || strstr(lib_file_name, "sb_cdres") != NULL) {
        if (!g_modules[1].in_use) {
            if (load_ne_file_into_slot(1, SB_HMODULE_SB_CDRES_DLL, "SB_CDRES.DLL", g_cdres_default_path) != 0) {
                return 0;
            }
        }
        return SB_HMODULE_SB_CDRES_DLL;
    }
    return 0;
}

void sb_win16_FreeLibrary(SB_HMODULE hModule)
{
    (void)hModule;
}

static const SBNeModule *find_module(SB_HMODULE hModule)
{
    size_t i;
    for (i = 0; i < SB_MAX_NE_MODULES; ++i) {
        if (g_modules[i].in_use && g_modules[i].handle == hModule) {
            return &g_modules[i];
        }
    }
    return NULL;
}

SB_HRSRC sb_win16_FindResource(SB_HMODULE hModule,
                               uint16_t res_id,
                               uint16_t res_type_id)
{
    const SBNeModule *mod = find_module(hModule);
    uint16_t norm_type = res_type_id & 0x7FFFu;
    uint16_t norm_id = res_id & 0x7FFFu;
    uint16_t i;

    if (!mod) {
        return 0;
    }
    for (i = 0; i < mod->res_count; ++i) {
        uint16_t idx = (uint16_t)(mod->first_res_index + i);
        const SBNeResourceEntry *e = &g_resources[idx];
        if (e->type_id == norm_type && e->res_id == norm_id) {
            /* Return 1-based HRSRC handle */
            return (SB_HRSRC)(idx + 1u);
        }
    }
    return 0;
}

SB_HGLOBAL sb_win16_LoadResource(SB_HMODULE hModule, SB_HRSRC hResInfo)
{
    uint16_t idx;
    (void)hModule;
    if (hResInfo == 0 || hResInfo > g_total_resources) {
        return 0;
    }
    idx = (uint16_t)(hResInfo - 1u);
    g_resources[idx].is_loaded = 1;
    /* In our shim, HGLOBAL for a resource uses the same 1-based index as HRSRC */
    return (SB_HGLOBAL)hResInfo;
}

const void *sb_win16_LockResource(SB_HGLOBAL hResData)
{
    uint16_t idx;
    const SBNeResourceEntry *e;
    const SBNeModule *mod;

    if (hResData == 0 || hResData > g_total_resources) {
        return NULL;
    }
    idx = (uint16_t)(hResData - 1u);
    e = &g_resources[idx];
    if (!e->is_loaded) {
        return NULL;
    }
    mod = find_module(e->hModule);
    if (!mod || !mod->file_bytes) {
        return NULL;
    }
    return mod->file_bytes + e->file_offset;
}

uint32_t sb_win16_SizeofResource(SB_HMODULE hModule, SB_HRSRC hResInfo)
{
    uint16_t idx;
    (void)hModule;
    if (hResInfo == 0 || hResInfo > g_total_resources) {
        return 0;
    }
    idx = (uint16_t)(hResInfo - 1u);
    return g_resources[idx].byte_length;
}

int sb_win16_FreeResource(SB_HGLOBAL hResData)
{
    uint16_t idx;
    if (hResData == 0 || hResData > g_total_resources) {
        return -1;
    }
    idx = (uint16_t)(hResData - 1u);
    g_resources[idx].is_loaded = 0;
    return 0;
}

int sb_win16_GlobalUnlock(SB_HGLOBAL hMem)
{
    (void)hMem;
    return 0;
}

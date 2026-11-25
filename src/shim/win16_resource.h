/* Thin Win16 API Shim — NE Resource & Module Loader (KERNEL)
 *
 * Provides clean, portable C implementations of the Win16 KERNEL resource and
 * module loading APIs used by original/BLIZZARD.EXE (`1008:2bbc..2bdc`,
 * `1010:0fe6`, `1010:286c`, `1010:3190`):
 *   - LoadLibrary / FreeLibrary
 *   - FindResource / LoadResource / LockResource / SizeofResource / FreeResource
 *   - GlobalUnlock (no-op for locked resource handles)
 *
 * Reads 16-bit Windows NE (New Executable) resource tables directly from
 * `original/BLIZZARD.EXE` and `original/SB_CDRES.DLL` on disk without modifying
 * either file.
 */
#ifndef SB_SHIM_WIN16_RESOURCE_H
#define SB_SHIM_WIN16_RESOURCE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t SB_HMODULE;
typedef uint16_t SB_HRSRC;
typedef uint16_t SB_HGLOBAL;

/* Well-known module handles matching BLIZZARD.EXE globals:
 * - DAT_1060_07d0: BLIZZARD.EXE application hInstance
 * - DAT_1060_001c: SB_CDRES.DLL loaded library HMODULE
 */
#define SB_HMODULE_BLIZZARD_EXE 0x0001u
#define SB_HMODULE_SB_CDRES_DLL 0x0002u

/* Initialize the Win16 NE resource shim by mounting the canonical
 * `BLIZZARD.EXE` and `SB_CDRES.DLL` paths (or default workspace paths if NULL).
 * Returns 0 on success, nonzero on error.
 */
int sb_win16_resource_shim_init(const char *blizzard_exe_path,
                                const char *sb_cdres_dll_path);

/* Release all memory loaded by the NE resource shim. */
void sb_win16_resource_shim_shutdown(void);

/* Win16 KERNEL API equivalents */
SB_HMODULE sb_win16_LoadLibrary(const char *lib_file_name);
void sb_win16_FreeLibrary(SB_HMODULE hModule);

/* FindResource by integer type ID (e.g. 0x03e8..0x0412 or 0x83e8..0x8412)
 * and integer resource ID (e.g. 1001, 1002, 501).
 */
SB_HRSRC sb_win16_FindResource(SB_HMODULE hModule,
                               uint16_t res_id,
                               uint16_t res_type_id);

SB_HGLOBAL sb_win16_LoadResource(SB_HMODULE hModule, SB_HRSRC hResInfo);
const void *sb_win16_LockResource(SB_HGLOBAL hResData);
uint32_t sb_win16_SizeofResource(SB_HMODULE hModule, SB_HRSRC hResInfo);
int sb_win16_FreeResource(SB_HGLOBAL hResData);
int sb_win16_GlobalUnlock(SB_HGLOBAL hMem);

#ifdef __cplusplus
}
#endif

#endif /* SB_SHIM_WIN16_RESOURCE_H */

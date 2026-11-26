/* Thin Win16 API Shim — GDI & USER Geometry, Palette, and 8-bit/1-bit Bitmap Engine
 *
 * Implements the Win16 GDI and USER rectangle/region/bitmap/palette APIs used
 * by original/BLIZZARD.EXE (`CPaletteManager_1008_8278`, `CPictSurface_1008_6448`,
 * `CLayeredOffscreen_1008_b310`, `1010:08e0..0fd0`, `1010:2ba8..2f6a`):
 *   - SetRect / IsRectEmpty / EqualRect / OffsetRect / IntersectRect / UnionRect
 *   - CreatePalette / SelectPalette / RealizePalette / AnimatePalette / GetSystemPaletteEntries
 *   - CreateCompatibleDC / DeleteDC / SelectObject / DeleteObject / GetObject
 *   - CreateBitmap / GetBitmapBits / SetBitmapBits / BitBlt / StretchBlt / FillRect
 */
#ifndef SB_SHIM_WIN16_GDI_H
#define SB_SHIM_WIN16_GDI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t SB_HDC;
typedef uint16_t SB_HBITMAP;
typedef uint16_t SB_HPALETTE;
typedef uint16_t SB_HGDIOBJ;

/* Standard Win16 Raster Operations (ROP3) used by BLIZZARD.EXE */
#define SB_SRCCOPY 0x00CC0020u
#define SB_SRCPAINT 0x00EE0086u
#define SB_SRCAND  0x008800C6u
#define SB_BLACKNESS 0x00000042u

/* Win16 16-bit signed RECT (8 bytes: left, top, right, bottom) */
typedef struct SB_RECT16 {
    int16_t left;   /* +0x00 */
    int16_t top;    /* +0x02 */
    int16_t right;  /* +0x04 */
    int16_t bottom; /* +0x06 */
} SB_RECT16;

/* Win16 16-bit POINT (4 bytes: x, y) */
typedef struct SB_POINT16 {
    int16_t x;      /* +0x00 */
    int16_t y;      /* +0x02 */
} SB_POINT16;

/* Win16 PALETTEENTRY (4 bytes) */
typedef struct SB_PALETTEENTRY {
    uint8_t peRed;
    uint8_t peGreen;
    uint8_t peBlue;
    uint8_t peFlags;
} SB_PALETTEENTRY;

/* Win16 LOGPALETTE header */
typedef struct SB_LOGPALETTE {
    uint16_t palVersion;    /* 0x0300 */
    uint16_t palNumEntries; /* e.g. 0xec (236) or 0x100 (256) */
    SB_PALETTEENTRY palPalEntry[256];
} SB_LOGPALETTE;

/* Win16 BITMAP info returned by GetObject (14 bytes / 0x0e in 1010:2d58) */
typedef struct SB_BITMAP16 {
    int16_t bmType;
    int16_t bmWidth;        /* +0x02 */
    int16_t bmHeight;       /* +0x04 */
    int16_t bmWidthBytes;   /* +0x06 */
    uint8_t bmPlanes;
    uint8_t bmBitsPixel;
    void *bmBits;
} SB_BITMAP16;

/* Reset/shutdown all GDI shim handles */
void sb_win16_gdi_shim_reset(void);

/* USER Rectangle APIs */
void sb_win16_SetRect(SB_RECT16 *rc, int16_t left, int16_t top, int16_t right, int16_t bottom);
int sb_win16_IsRectEmpty(const SB_RECT16 *rc);
int sb_win16_EqualRect(const SB_RECT16 *a, const SB_RECT16 *b);
void sb_win16_OffsetRect(SB_RECT16 *rc, int16_t dx, int16_t dy);
int sb_win16_IntersectRect(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b);
int sb_win16_UnionRect(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b);

/* GDI Palette APIs */
uint16_t sb_win16_GetSystemPaletteEntries(SB_HDC hdc,
                                          uint16_t iStartIndex,
                                          uint16_t nEntries,
                                          SB_PALETTEENTRY *lppe);
SB_HPALETTE sb_win16_CreatePalette(const SB_LOGPALETTE *lplgpl);
SB_HPALETTE sb_win16_SelectPalette(SB_HDC hdc, SB_HPALETTE hpal, int bForceBackground);
uint16_t sb_win16_RealizePalette(SB_HDC hdc);
void sb_win16_AnimatePalette(SB_HPALETTE hpal,
                             uint16_t iStartIndex,
                             uint16_t nEntries,
                             const SB_PALETTEENTRY *lppe);
const SB_PALETTEENTRY *sb_win16_gdi_getActivePaletteEntries(void);

/* GDI Device Context & Bitmap APIs */
SB_HDC sb_win16_CreateCompatibleDC(SB_HDC hdc);
int sb_win16_DeleteDC(SB_HDC hdc);
SB_HGDIOBJ sb_win16_SelectObject(SB_HDC hdc, SB_HGDIOBJ hgdiobj);
int sb_win16_DeleteObject(SB_HGDIOBJ hgdiobj);
int sb_win16_GetObjectBitmap(SB_HBITMAP hbm, int cbBuffer, SB_BITMAP16 *lpvObject);
uint32_t sb_win16_SetBkColor(SB_HDC hdc, uint32_t crColor);

SB_HBITMAP sb_win16_CreateBitmap(int16_t nWidth,
                                 int16_t nHeight,
                                 uint8_t cPlanes,
                                 uint8_t cBitsPerPel,
                                 const void *lpvBits);
int32_t sb_win16_GetBitmapBits(SB_HBITMAP hbmp, int32_t cbBuffer, void *lpvBits);
int32_t sb_win16_SetBitmapBits(SB_HBITMAP hbmp, int32_t cBytes, const void *lpvBits);
const uint8_t *sb_win16_gdi_getBitmapPixels(SB_HBITMAP hbmp, int16_t *out_w, int16_t *out_h);

int sb_win16_FillRectColor(SB_HDC hdc, const SB_RECT16 *lprc, uint8_t color_index);
int sb_win16_BitBlt(SB_HDC hdcDest,
                    int16_t nXDest,
                    int16_t nYDest,
                    int16_t nWidth,
                    int16_t nHeight,
                    SB_HDC hdcSrc,
                    int16_t nXSrc,
                    int16_t nYSrc,
                    uint32_t dwRop);
int sb_win16_StretchBlt(SB_HDC hdcDest,
                        int16_t nXOriginDest,
                        int16_t nYOriginDest,
                        int16_t nWidthDest,
                        int16_t nHeightDest,
                        SB_HDC hdcSrc,
                        int16_t nXOriginSrc,
                        int16_t nYOriginSrc,
                        int16_t nWidthSrc,
                        int16_t nHeightSrc,
                        uint32_t dwRop);

#ifdef __cplusplus
}
#endif

#endif /* SB_SHIM_WIN16_GDI_H */

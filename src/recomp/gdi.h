/* Host GDI: device-dependent 8-bit palette display emulation. */
#ifndef SB_RECOMP_GDI_H
#define SB_RECOMP_GDI_H

#include <stdint.h>

#define SCREEN_W 640
#define SCREEN_H 480

typedef struct RGBQ {
    uint8_t r, g, b, flags;
} RGBQ;

/* The physical display: 8-bit indices into the system palette. */
extern uint8_t g_screen[SCREEN_W * SCREEN_H];
extern RGBQ g_syspal[256];
extern int g_screen_dirty;

/* Window-DC binding provided by USER: returns 0 if hwnd invalid. Fills the
 * client origin in screen coordinates and the client size. */
int user_client_area(uint16_t hwnd, int *x, int *y, int *w, int *h);
/* Called by USER when a window DC is created. */
uint16_t gdi_create_window_dc(uint16_t hwnd);
void gdi_release_window_dc(uint16_t hdc);
/* Fill a screen rectangle (window background erase). */
void gdi_fill_screen_rect(int x0, int y0, int x1, int y1, uint32_t colorref_or_brush, int is_brush);
uint32_t gdi_brush_color(uint16_t hbr, int *ok);

void gdi_init(const char *font_dir);
void gdi_shutdown(void);

#endif

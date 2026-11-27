/* Host-side Win16 subsystem (USER/GDI/MMSYSTEM over SDL2). */
#ifndef SB_RECOMP_WIN16_H
#define SB_RECOMP_WIN16_H

#include <stddef.h>
#include <stdint.h>

typedef struct Win16Options {
    int scale;
    int audio;
    int headless;          /* dummy SDL drivers */
    int fullscreen;        /* start full screen (desktop resolution) */
    uint32_t max_ms;       /* auto-quit after this many ms (0 = never) */
    const char *snapshot;  /* write final screen as PPM here */
    const char *script;    /* scripted input: "t=ms key|char|click ..." */
} Win16Options;

int win16_init(const Win16Options *opt);
void win16_shutdown(void);

/* string helpers (kernel.c) */
size_t far_strlen(uint32_t fp);
void far_to_c(uint32_t fp, char *out, size_t cap);
void c_to_far(const char *s, uint32_t fp, size_t cap);
int kernel_find_resource_raw(uint16_t hmod, uint16_t type_id, uint16_t name_id, const uint8_t **data, uint32_t *len);

#endif

/* SDL2-specific entry points of the platform backend. */
#ifndef SB_PLATFORM_SDL_H
#define SB_PLATFORM_SDL_H

#include <SDL.h>

/* Translates one SDL event into Win16 messages for the target HWND.
 * `sb_platform_pump_events` calls this for every polled event; tests call it
 * directly with synthetic events. */
void sb_platform_sdl_handle_event(const SDL_Event *ev);

#endif /* SB_PLATFORM_SDL_H */

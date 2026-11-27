/* Fallbacks for imported APIs not implemented yet: each stops the game with
 * the API's name. The real implementations (kernel.c, user.c, gdi.c,
 * mmsystem.c) override these weak definitions. */
#include "api.h"

#define X(idx, mod, name, ord, purge) \
    __attribute__((weak)) void api_##mod##_##name(void) { api_unimplemented(#mod, #name, ord); }
#include "import_list.h"
#undef X

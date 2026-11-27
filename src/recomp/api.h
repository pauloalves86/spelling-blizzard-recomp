/* Win16 API layer entry points called by recompiled code. */
#ifndef SB_RECOMP_API_H
#define SB_RECOMP_API_H

#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "api_decls.h"

_Noreturn void api_unimplemented(const char *module, const char *name, int ordinal);
void api_log(const char *fmt, ...);
extern int g_api_trace;

#ifdef __cplusplus
}
#endif

#endif

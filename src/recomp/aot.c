/* The program compiled ahead of time: the generated C (recomp_gen/) built
 * with the host compiler into this executable (sb_recomp_aot, for
 * development: a debugger sees every f_SSSS_OOOO). */
#include "runtime.h"

extern const RecompEntry recomp_table[];
extern const unsigned recomp_table_count;
void recomp_start(void);

int recomp_load_program(const char *exe_path, char *err, size_t errcap)
{
    (void)exe_path;
    (void)err;
    (void)errcap;
    recomp_program.table = recomp_table;
    recomp_program.count = recomp_table_count;
    recomp_program.start = recomp_start;
    return 0;
}

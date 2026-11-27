/* Static recompiler: BLIZZARD.EXE (Win16 NE, x86-16) -> C, in memory.
 *
 * Every function becomes `void f_SSSS_OOOO(void)` operating on the emulated CPU
 * (src/recomp/cpu.h). Branches become gotos, near/far calls push the original
 * return address and call the target C function, `ret`/`retf` pop it and
 * return. Relocations are resolved at generation time (selector immediates,
 * far calls to other segments, far calls to imported APIs). Indirect far calls
 * (C++ virtual calls) go through `call_far`, backed by a generated lookup table.
 */
#ifndef SB_GEN_GEN_H
#define SB_GEN_GEN_H

#include <stddef.h>
#include <stdint.h>

typedef struct GenFile {
    char name[32];
    char *data;
    size_t len;
} GenFile;

typedef struct GenOutput {
    /* recomp_gen.h, gen_SSSS_K.c..., gen_table.c */
    GenFile *files;
    int nfiles;
    /* The text of src/recomp/imports/import_list.h and api_decls.h for this EXE:
     * the runtime is compiled against the checked-in copies, which must match. */
    GenFile import_list, api_decls;
    int nfuncs;            /* functions generated */
    int nimports_called;   /* imported APIs called directly */
    int nunresolved;       /* call targets reached only through call_far */
} GenOutput;

/* The function list from the Ghidra analysis (functions.txt), built in. */
extern const char gen_functions_txt[];

/* Recompiles the NE executable in exe. functions_txt lists known function
 * entry points ("SSSS:OOOO" per line, '#' comments); NULL uses the built-in
 * list. Returns 0, or -1 with a message in err. */
int gen_recompile(const uint8_t *exe, size_t len, const char *functions_txt, GenOutput *out, char *err,
                  size_t errcap);
void gen_output_free(GenOutput *out);

#endif

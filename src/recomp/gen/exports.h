/* Win16 API export table used to name imported functions. */
#ifndef SB_GEN_EXPORTS_H
#define SB_GEN_EXPORTS_H

typedef struct GenExport {
    const char *module;
    int ordinal;
    const char *name;
    int purge;
} GenExport;

/* Sorted by module, then ordinal. */
extern const GenExport gen_exports[];
extern const unsigned gen_export_count;

#endif

/* Minimal NE (New Executable) reader for the static recompiler: the header,
 * segment table, per-segment relocations (chains walked), entry table and
 * module reference table. Works on a buffer; never writes to it. */
#ifndef SB_GEN_NE_H
#define SB_GEN_NE_H

#include <stddef.h>
#include <stdint.h>

enum { NE_T_INTERNAL, NE_T_MOVABLE, NE_T_ORD, NE_T_NAME, NE_T_OSFIX };

typedef struct NEReloc {
    uint8_t src_type;   /* 0 lobyte, 2 selector, 3 far ptr (sel:off), 5 offset, 11 ptr48, 13 off32 */
    uint8_t flags;      /* bits 0-1 target type, bit 2 additive */
    uint8_t ttype;      /* NE_T_* */
    uint16_t seg;       /* internal: target segment */
    uint16_t off;       /* internal: target offset; movable: entry ordinal; ord: ordinal */
    uint16_t module;    /* ord/name: 0-based module index */
    uint32_t *locs;     /* every patched location (chain walked) */
    int nlocs;
} NEReloc;

typedef struct NESegment {
    uint32_t file_off;
    uint32_t length;    /* bytes present in the file */
    uint16_t flags;
    const uint8_t *data;
    size_t datalen;
    NEReloc *relocs;
    int nrelocs;
} NESegment;

typedef struct NEEntry {
    uint16_t ordinal;
    uint8_t seg;
    uint16_t off;
} NEEntry;

typedef struct NEFile {
    uint16_t cs, ip;
    int nseg;
    NESegment *segs;    /* [1..nseg] */
    int nmod;
    char **modules;
    NEEntry *entries;
    int nentries;
} NEFile;

/* Parses the file in buf (which must outlive it). Returns 0, or -1 with a message. */
int ne_parse(NEFile *ne, const uint8_t *buf, size_t len, char *err, size_t errcap);
void ne_free(NEFile *ne);

/* Runtime selector == Ghidra segment value for BLIZZARD.EXE segment N. */
static inline unsigned ne_sel_of(int segnum) { return 0x1000u + (unsigned)(segnum - 1) * 8u; }

#endif

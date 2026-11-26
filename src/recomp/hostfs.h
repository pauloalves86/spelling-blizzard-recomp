/* Mapping of DOS paths used by the game onto host directories.
 *
 * The 1994 game was installed to a hard-disk directory (EXE, INI, player
 * records) and read SB_CDRES.DLL from the CD. Here a DOS path is resolved by
 * its file name: first in the writable data directory (saves, INI), then in
 * the read-only game directory (original/). New files go to the data dir.
 */
#ifndef SB_RECOMP_HOSTFS_H
#define SB_RECOMP_HOSTFS_H

#include <stddef.h>

void hostfs_init(const char *game_dir, const char *data_dir);
/* Resolve for reading: returns 0 and fills out if the file exists. */
int hostfs_resolve_existing(const char *dos_path, char *out, size_t cap);
/* Resolve for writing/creating (always in the data dir). */
void hostfs_resolve_create(const char *dos_path, char *out, size_t cap);
const char *hostfs_data_dir(void);
/* Resolve a directory in the data dir (0 if it exists). */
int hostfs_resolve_dir(const char *dos_dir, char *out, size_t cap);

#endif

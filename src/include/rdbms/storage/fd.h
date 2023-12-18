// =========================================================================
//
// fd.h
//  Virtual file descriptor definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: fd.h,v 1.21 2000/04/12 17:16:51 momjian Exp $
//
// =========================================================================

// calls:
//
//  File {Close, Read, Write, Seek, Tell, MarkDirty, Sync}
//  {File Name Open, Allocate, Free} File
//
// These are NOT JUST RENAMINGS OF THE UNIX ROUTINES.
// Use them for all file activity...
//
//  File fd;
//  fd = FilePathOpenFile("foo", O_RDONLY);
//
//  AllocateFile();
//  FreeFile();
//
// Use AllocateFile, not fopen, if you need a stdio file (FILE*); then
// use FreeFile, not fclose, to close it.  AVOID using stdio for files
// that you intend to hold open for any length of time, since there is
// no way for them to share kernel file descriptors with other files.

#ifndef RDBMS_STORAGE_FD_H_
#define RDBMS_STORAGE_FD_H_

#include <stdbool.h>
#include <stdio.h>

// FileSeek uses the standard UNIX lseek(2) flags.

typedef char* FileName;
typedef int File;

// Operations on virtual Files --- equivalent to Unix kernel file ops.
File file_name_open_file(FileName filename, int file_flags, int file_mode);
File path_name_open_file(FileName filename, int file_flags, int file_mode);
File open_temporary_file(void);
void file_close(File file);
void file_unlink(File file);
int file_read(File file, char* buffer, int amount);
int file_write(File file, char* buffer, int amount);
long file_seek(File file, long offset, int whence);
int file_truncate(File file, long offset);
int file_sync(File file);
void file_mark_dirty(File file);

// Operations that allow use of regular stdio --- USE WITH CAUTION.
FILE* allocate_file(char* name, char* mode);
void free_file(FILE*);

// Miscellaneous support routines.
bool release_data_file(void);
void close_all_vfds(void);
void at_eo_xact_files(void);
int pg_fsync(int fd);

#endif  // RDBMS_STORAGE_FD_H_

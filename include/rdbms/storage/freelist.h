#ifndef RDBMS_STORAGE_FREELIST_H_
#define RDBMS_STORAGE_FREELIST_H_

#include "rdbms/storage/buf_internals.h"

extern int Free_List_Descriptor;
extern BufferDesc* BufferDescriptors;
extern long* PrivateRefCount;

void add_buffer_to_freelist(BufferDesc* buf_desc);
void pin_buffer(BufferDesc* buf_desc);
void pin_buffer_debug(char* file, int line, BufferDesc* buf_desc);
void unpin_buffer(BufferDesc* buf_desc);
BufferDesc* get_free_buffer(void);
void init_freelist(bool init);

#endif  // RDBMS_STORAGE_FREELIST_H_

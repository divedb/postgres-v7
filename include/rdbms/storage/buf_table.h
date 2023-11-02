#ifndef RDBMS_STORAGE_BUF_TABLE_H_
#define RDBMS_STORAGE_BUF_TABLE_H_

#include "rdbms/storage/buf_internals.h"

void init_buf_table(void);
BufferDesc* buf_table_lookup(BufferTag* tag_ptr);
bool buf_table_delete(BufferDesc* buf_desc);
bool buf_table_insert(BufferDesc* buf_desc);

#endif  // RDBMS_STORAGE_BUF_TABLE_H_

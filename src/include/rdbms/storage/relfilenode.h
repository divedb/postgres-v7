#ifndef RDBMS_STORAGE_REL_FILE_NODE_H_
#define RDBMS_STORAGE_REL_FILE_NODE_H_

#include "rdbms/postgres.h"

// This is all what we need to know to find relation file.
// tblNode is identificator of tablespace and because of
// currently our tablespaces are equal to databases this is
// database OID. relNode is currently relation OID on creation
// but may be changed later if required. relNode is stored in
// pg_class.relfilenode.
// 根据数据库和表的oid就能定位到文件
typedef struct RelFileNode {
  Oid tbl_node;  // tablespace.
  Oid rel_node;  // relation
} RelFileNode;

#define REL_FILE_NODE_EQUALS(node1, node2) \
  ((node1).rel_node == (node2).rel_node && (node1).tbl_node == (node2).tbl_node)

#endif  // RDBMS_STORAGE_REL_FILE_NODE_H_

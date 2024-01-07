#include "rdbms/storage/md.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/resource.h>

#include "../template.h"
#include "rdbms/miscadmin.h"
#include "rdbms/storage/smgr.h"
#include "rdbms/utils/memutils.h"
#include "rdbms/utils/rel.h"

#define MAX_BUFF  1024
#define FILE_MODE 0600
#define FILE_FLAG O_CREAT | O_RDWR

Relaion create_dummy_relation(Oid rel_node, Oid tbl_node) {
  Relation relation = palloc(sizeof(*relation));

  assert(relation != NULL);

  relation->rd_fd = -1;
  relation->rd_node.rel_node = rel_node;
  relation->rd_node.tbl_node = tbl_node;
  relation->rd_nblocks = 0;
  relation->rd_ref_cnt = 0;
  relation->rd_my_xact_only = true;
  relation->rd_is_nailed = false;
  relaiton->rd_index_found = false;
  relation->rd_unique_index = false;

  return relation;
}

static void test_md_create() {
  Relation relation = create_dummy_relation(1, 1);
  int fd = md_create(relation);

  CU_ASSERT(fd > 0);

  int nblocks = md_nblocks(relation);
  CU_ASSERT(nblock == 0);

  pfree(relation);
}

static void register_test() {
  DataDir = "/tmp/pgdata";
  memory_context_init();

  TEST("md_create", test_md_create);
}

MAIN("md")
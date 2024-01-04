#include "rdbms/storage/bufmgr.h"

#include "rdbms/access/xlogdefs.h"
#include "rdbms/storage/buf_internals.h"

#define BUFFER_GET_LSN(buf_hdr) (*((XLogRecPtr*)MAKE_PTR((buf_hdr)->data)))

extern SpinLock BufMgrLock;
extern long int ReadBufferCount;
extern long int ReadLocalBufferCount;
extern long int BufferHitCount;
extern long int LocalBufferHitCount;
extern long int BufferFlushCount;
extern long int LocalBufferFlushCount;

// It's used to avoid disk writes for read-only transactions
// (i.e. when no one shared buffer was changed by transaction).
// We set it to true in WriteBuffer/WriteNoReleaseBuffer when
// marking shared buffer as dirty. We set it to false in xact.c
// after transaction is committed/aborted.
bool SharedBufferChanged = false;

static void wait_io(BufferDesc* buf, SpinLock spinlock);
static void start_buffer_io(BufferDesc* buf, bool forInput);
static void terminate_buffer_io(BufferDesc* buf);
static void continue_buffer_io(BufferDesc* buf, bool forInput);
extern void abort_buffer_io(void);

// Note that write error doesn't mean the buffer broken.
#define BUFFER_IS_BROKEN(buf) \
  ((buf->flags & BM_IO_ERROR) && !(buf->flags & BM_DIRTY))

static Buffer read_buffer_with_buffer_lock(Relation relation,
                                           BlockNumber block_number,
                                           bool buffer_lock_held);

Buffer relation_get_buffer_write_buffer(Relation relation,
                                        BlockNumber block_number,
                                        Buffer buffer) {
  BufferDesc* buf_hdr;

  if (BUFFER_IS_VALID(buffer)) {
    if (!BUFFER_IS_LOCAL(buffer)) {
      buf_hdr = &BufferDescriptors[buffer - 1];
      spin_acquire(BufMgrLock);

      if (buf_hdr->tag.block_num == block_number &&
          REL_FILE_NODE_EQUALS(buf_hdr->tag.rnode, relation->rd_node)) {
        spin_release(BufMgrLock);
        return buffer;
      }

      return read_buffer_with_buffer_lock(relation, block_number, true);
    }
  }
}

// Does the work of ReadBuffer() but with the possibility that the buffer lock
// has already been held. this is yet another effort to reduce the number of
// semops in the system.
static Buffer read_buffer_with_buffer_lock(Relation relation,
                                           BlockNumber block_number,
                                           bool buffer_lock_held) {
  BufferDesc* buf_hdr;
  int extend;  // Extending the file by one block
  int status;
  bool found;
  bool is_local_buf;

  extend = (block_number == P_NEW);
  is_local_buf = relation->rd_my_xact_only;

  if (is_local_buf) {
    ReadLocalBufferCount++;
    buf_hdr = local_buffer_alloc(relation, block_num, &found);
  }
}
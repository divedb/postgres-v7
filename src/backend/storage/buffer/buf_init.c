#include "rdbms/storage/buf_init.h"

// If BMTRACE is defined, we trace the last 200 buffer allocations and
// deallocations in a circular buffer in shared memory.
#ifdef BMTRACE
bmtrace* TraceBuf;
long* CurTraceBuf;

#define BMT_LIMIT 200
#endif  // BMTRACE

int ShowPinTrace = 0;
// int NBuffers = DEF_NBUFFERS; // default is set in config.h
int NBuffers = 16;
int Data_Descriptors;
int Free_List_Descriptor;
int Lookup_List_Descriptor;
int Num_Descriptors;

long int ReadBufferCount;
long int ReadLocalBufferCount;
long int BufferHitCount;
long int LocalBufferHitCount;
long int BufferFlushCount;
long int LocalBufferFlushCount;

// Initialize module:
//
// should calculate size of pool dynamically based on the
// amount of available memory.
void init_buffer_pool(IPCKey key) {
  bool found_bufs;
  bool found_descs;
  int i;
}
//===----------------------------------------------------------------------===//
//
// libpq.h
//  POSTGRES LIBPQ buffer structure definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: libpq.h,v 1.36 2000/04/12 17:16:36 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_LIBPQ_LIBPQ_H_
#define RDBMS_LIBPQ_LIBPQ_H_

#include <sys/types.h>

#include "rdbms/access/htup.h"
#include "rdbms/access/tupdesc.h"
#include "rdbms/lib/stringinfo.h"
#include "rdbms/postgres.h"
#include "rdbms/tcop/dest.h"

// TODO(gc): no definition provided.

typedef struct {
  int len;
  bool is_int;
  union {
    void* ptr;
    int integer;
  } u;
} PQArgBlock;

typedef struct TypeBlock {
  char name[NAME_DATA_LEN];  // Name of the attribute.
  int type_id;               // Type id of the type.
  int type_len;              // Type len of the type.
} TypeBlock;

// Data of a tuple.
#define TUPLE_BLOCK_SIZE 100

typedef struct TupleBlock {
  char** values[TUPLE_BLOCK_SIZE];  // An array of tuples.
  int* lengths[TUPLE_BLOCK_SIZE];   // An array of length vec for each tuple.
  struct TupleBlock* next;          // Next tuple block.
  int tuple_index;                  // Current tuple index.
} TupleBlock;

// A group of tuples with the same attributes.
typedef struct GroupBuffer {
  int no_tuples;             // Number of tuples in this group.
  int no_fields;             // Number of attributes.
  TypeBlock* types;          // Types of the attributes.
  TupleBlock* tuples;        // Tuples in this group.
  struct GroupBuffer* next;  // Next group.
} GroupBuffer;

// Data structure of a portal buffer.
typedef struct PortalBuffer {
  int rule_p;           // 1 if this is an asynchronized portal.
  int no_tuples;        // Number of tuples in this portal buffer.
  int no_groups;        // Number of tuple groups.
  GroupBuffer* groups;  // Linked list of tuple groups.
} PortalBuffer;

// An entry in the global portal table
//
// Note: the portalcxt is only meaningful for PQcalls made from
//       within a postgres backend.  frontend apps should ignore it.
#define PORTAL_NAME_LENGTH 32

typedef struct PortalEntry {
  char name[PORTAL_NAME_LENGTH];  // Name of this portal.
  PortalBuffer* portal;           // Tuples contained in this portal.
  Pointer portalcxt;              // Memory context (for backend).
  Pointer result;                 // Result for PQexec.
} PortalEntry;

#define PORTALS_INITIAL_SIZE 32
#define PORTALS_GROW_BY      32

// In portalbuf.c
extern PortalEntry** Portals;
extern size_t PortalsArraySize;

// Exceptions.
#define LIBPQ_RAISE(x, y) ExcRaise((Exception*)(x), (ExcDetail)(y), (ExcData)0, (ExcMessage)0)

// In portal.c
extern Exception MemoryError;
extern Exception PortalError;
extern Exception PostquelError;
extern Exception ProtocolError;

// PQErrorMsg[] is used only for error messages generated within backend
// libpq, none of which are remarkably long.  Note that this length should
// NOT be taken as any indication of the maximum error message length that
// the backend can create! elog() can in fact produce extremely long messages.
#define PQ_ERROR_MSG_LENGTH 1024

// In libpq/util.c
extern char PQErrorMsg[PQ_ERROR_MSG_LENGTH];

// Prototypes for functions in portal.c
void pq_debug(char* target, char* msg);
void pq_debug2(char* target, char* msg1, char* msg2);
void pq_trace(void);
void pq_untrace(void);
int pq_nportals(int rule_p);
void pq_pnames(char** pnames, int rule_p);
PortalBuffer* pq_parray(char* pname);
int pq_rulep(PortalBuffer* portal);
int pq_ntuples(PortalBuffer* portal);
int pq_ninstances(PortalBuffer* portal);
int pq_ngroups(PortalBuffer* portal);
int pq_ntuples_group(PortalBuffer* portal, int group_index);
int pq_ninstances_group(PortalBuffer* portal, int group_index);
int pq_nfields_group(PortalBuffer* portal, int group_index);
int pq_fnumber_group(PortalBuffer* portal, int group_index, char* field_name);
char* pq_fname_group(PortalBuffer* portal, int group_index, int field_number);
int pq_ftype_group(PortalBuffer* portal, int group_index, int field_number);
int pq_fsize_group(PortalBuffer* portal, int group_index, int field_number);
GroupBuffer* pq_group(PortalBuffer* portal, int tuple_index);
int pq_get_group(PortalBuffer* portal, int tuple_index);
int pq_nfields(PortalBuffer* portal, int tuple_index);
int pq_fnumber(PortalBuffer* portal, int tuple_index, char* field_name);
char* pq_fname(PortalBuffer* portal, int tuple_index, int field_number);
int pq_ftype(PortalBuffer* portal, int tuple_index, int field_number);
int pq_fsize(PortalBuffer* portal, int tuple_index, int field_number);
int pq_same_type(PortalBuffer* portal, int tuple_index1, int tuple_index2);
char* pq_get_value(PortalBuffer* portal, int tuple_index, int field_number);
char* pq_get_attr(PortalBuffer* portal, int tuple_index, int field_number);
int pq_get_length(PortalBuffer* portal, int tuple_index, int field_number);
void pq_clear(char* pname);

// Prototypes for functions in portalbuf.c
caddr_t pbuf_alloc(size_t size);
void pbuf_free(caddr_t pointer);
PortalBuffer* pbuf_add_portal(void);
GroupBuffer* pbuf_add_group(PortalBuffer* portal);
TypeBlock* pbuf_add_types(int n);
TupleBlock* pbuf_add_tuples(void);
char** pbuf_add_tuple(int n);
int* pbuf_add_tuple_value_lengths(int n);
char* pbuf_add_values(int n);
PortalEntry* pbuf_add_entry(void);
void pbuf_free_entry(int i);
void pbuf_free_types(TypeBlock* types);
void pbuf_free_tuples(TupleBlock* tuples, int no_tuples, int no_fields);
void pbuf_free_group(GroupBuffer* group);
void pbuf_free_portal(PortalBuffer* portal);
int pbuf_get_index(char* pname);
void pbuf_set_portal_info(PortalEntry* entry, char* pname);
PortalEntry* pbuf_setup(char* pname);
void pbuf_close(char* pname);
GroupBuffer* pbuf_find_group(PortalBuffer* portal, int group_index);
int pbuf_find_fnumber(GroupBuffer* group, char* field_name);
void pbuf_check_fnumber(GroupBuffer* group, int field_number);
char* pbuf_find_fname(GroupBuffer* group, int field_number);

// In be-dumpdata.c
void be_portal_init(void);
void be_portal_push(PortalEntry* entry);
PortalEntry* be_portal_pop(void);
PortalEntry* be_current_portal(void);
PortalEntry* be_new_portal(void);
void be_type_init(PortalEntry* entry, TupleDesc attrs, int natts);
void be_print_tup(HeapTuple tuple, TupleDesc typeinfo, DestReceiver* self);

// Prototypes for functions in be-pqexec.c
char* pq_fn(int fnid, int* result_buf, int result_len, int result_is_int, PQArgBlock* args, int nargs);
char* pq_exec(char* query);
int pq_test_pq_exec(char* q);
int pq_test_pq_fn(char* q);
int32 pq_test(struct varlena* vlena);

// Prototypes for functions in pqcomm.c
int stream_sever_port(char* hostname, unsigned short port, int* fdp);
int stream_connection(int server_fd, Port* port);
void stream_close(int sock);
void pq_init();
int pq_get_port();
void pq_close();
extern int pq_get_bytes(char* s, size_t len);
extern int pq_get_string(StringInfo s);
extern int pq_peek_byte(void);
extern int pq_put_bytes(const char* s, size_t len);
extern int pq_flush(void);
extern int pq_put_message(char msg_type, const char* s, size_t len);
extern void pq_start_copy_out(void);
extern void pq_end_copy_out(bool error_abort);

#endif  // RDBMS_LIBPQ_LIBPQ_H_

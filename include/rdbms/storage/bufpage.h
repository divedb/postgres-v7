// =========================================================================
//
// bufpage.h
//  Standard POSTGRES buffer page definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: bufpage.h,v 1.28 2000/01/26 05:58:32 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_BUF_PAGE_H_
#define RDBMS_STORAGE_BUF_PAGE_H_

#include "rdbms/storage/item.h"
#include "rdbms/storage/itemid.h"
#include "rdbms/storage/off.h"
#include "rdbms/storage/page.h"
#include "rdbms/utils/palloc.h"

// A postgres disk page is an abstraction layered on top of a postgres
// disk block (which is simply a unit of i/o, see block.h).
//
// specifically, while a disk block can be unformatted, a postgres
// disk page is always a slotted page of the form:
//
// +----------------+---------------------------------+
// | PageHeaderData | linp1 linp2 linp3 ...           |
// +-----------+----+---------------------------------+
// | ... linpN |                                      |
// +-----------+--------------------------------------+
// |           ^ pd_lower                             |
// |                                                  |
// |             v pd_upper                           |
// +-------------+------------------------------------+
// |             | tupleN ...                         |
// +-------------+------------------+-----------------+
// |       ... tuple3 tuple2 tuple1 | "special space" |
// +--------------------------------+-----------------+
//                                  ^ pd_special
//
// a page is full when nothing can be added between pd_lower and
// pd_upper.
//
// all blocks written out by an access method must be disk pages.
//
// EXCEPTIONS:
//
// obviously, a page is not formatted before it is initialized with by
// a call to PageInit.
//
// the contents of the special pg_variable/pg_time/pg_log tables are
// raw disk blocks with special formats.  these are the only "access
// methods" that need not write disk pages.
//
// NOTES:
//
// linp1..N form an ItemId array.  ItemPointers point into this array
// rather than pointing directly to a tuple.  Note that OffsetNumbers
// conventionally start at 1, not 0.
//
// tuple1..N are added "backwards" on the page.  because a tuple's
// ItemPointer points to its ItemId entry rather than its actual
// byte-offset position, tuples can be physically shuffled on a page
// whenever the need arises.
//
// AM-generic per-page information is kept in the pd_opaque field of
// the PageHeaderData. (Currently, only the page size is kept here.)
//
// AM-specific per-page data (if any) is kept in the area marked "special
// space"; each AM has an "opaque" structure defined somewhere that is
// stored as the page trailer. an access method should always
// initialize its pages with PageInit and then set its own opaque
// fields.

#define PAGE_IS_VALID(page) POINTER_IS_VALID(page)

// Location (byte offset) within a page.
//
// Note that this is actually limited to 2^15 because we have limited
// ItemIdData.lp_off and ItemIdData.lp_len to 15 bits (see itemid.h).
typedef uint16 LocationIndex;

// space management information generic to any page
//
// od_pagesize -    size in bytes.
//                  Minimum possible page size is perhaps 64B to fit
//                  page header, opaque space and a minimal tuple;
//                  of course, in reality you want it much bigger.
//                  On the high end, we can only support pages up
//                  to 32KB because lp_off/lp_len are 15 bits.
typedef struct OpaqueData {
  uint16 od_pagesize;
} OpaqueData;

typedef OpaqueData* Opaque;

// Disk page organization
typedef struct PageHeaderData {
  LocationIndex pd_lower;    // Offset to start of free space.
  LocationIndex pd_upper;    // Offset to end of free space.
  LocationIndex pd_special;  // Offset to start of special space.
  OpaqueData pd_opaque;      // AM-generic information.
  ItemIdData pd_linp[1];     // Beginning of line pointer array.
} PageHeaderData;

typedef PageHeaderData* PageHeader;

typedef enum { ShufflePageManagerMode, OverwritePageManagerMode } PageManagerMode;

#define PAGE_IS_USED(page)                 (assert(PAGE_IS_VALID(page)), ((bool)(((PageHeader)(page))->pd_lower != 0)))
#define PAGE_IS_EMPTY(page)                (((PageHeader)(page))->pd_lower <= (sizeof(PageHeaderData) - sizeof(ItemIdData)))
#define PAGE_IS_NEW(page)                  (((PageHeader)(page))->pd_upper == 0)
#define PAGE_GET_ITEM_ID(page, offset_num) ((ItemId)(&((PageHeader)(page))->pd_linp[(offset_num)-1]))
#define PAGE_SIZE_IS_VALID(page_size)      ((page_size) == BLCKSZ)
#define PAGE_GET_PAGE_SIZE(page)           ((Size)((PageHeader)(page))->pd_opaque.od_pagesize)
#define PAGE_SET_PAGE_SIZE(page, size)     (((PageHeader)(page))->pd_opaque.od_pagesize = (size))
#define PAGE_GET_SPECIAL_SIZE(page)        ((uint16)(PageGetPageSize(page) - ((PageHeader)(page))->pd_special))
#define PAGE_GET_SPECIAL_POINTER(page) \
  (assert(PAGE_IS_VALID(page)), (char*)((char*)(page) + ((PageHeader)(page))->pd_special))
#define PAGE_GET_ITEM(page, itemid) \
  (assert(PAGE_IS_VALID(page)), assert((itemid)->lp_flags& LP_USED), (Item)(((char*)(page)) + (itemid)->lp_off))
#define BUFFER_GET_PAGE_SIZE(buffer) (assert(BUFFER_IS_VALID(buffer)), (Size)BLCKSZ)
#define BUFFER_GET_PAGE(buffer)      ((Page)BUFFER_GET_BLOCK(buffer))
#define PAGE_GET_MAX_OFFSET_NUMBER(page) \
  (((int)(((PageHeader)(page))->pd_lower - (sizeof(PageHeaderData) - sizeof(ItemIdData)))) / ((int)sizeof(ItemIdData)))

void page_init(Page page, Size page_size, Size special_size);
OffsetNumber page_add_item(Page page, Item item, Size size, OffsetNumber offset_num, ItemIdFlags flags);
Page page_get_temp_page(Page page, Size special_size);
void page_restore_temp_page(Page temp_page, Page old_page);
void page_repair_fragmentation(Page page);
Size page_get_free_space(Page page);
void page_manager_mode_set(PageManagerMode mode);
void page_index_tuple_delete(Page page, OffsetNumber offset_num);

#endif  // RDBMS_STORAGE_BUF_PAGE_H_

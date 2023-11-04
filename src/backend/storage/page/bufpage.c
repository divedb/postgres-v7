// =========================================================================
//
// bufpage.c
//  POSTGRES standard buffer page code.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//	  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/page/bufpage.c,v 1.29 2000/04/12 17:15:40 momjian Exp $
//
// =========================================================================

#include "rdbms/storage/bufpage.h"

#include <stdbool.h>
#include <sys/file.h>
#include <sys/types.h>

#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/memutils.h"

static bool PageManagerShuffle = true;  // Default is shuffle mode.

static void page_index_tuple_delete_adjust_line_pointers(PageHeader phdr, char* location, Size size);

// Initializes the contents of a page.
void page_init(Page page, Size page_size, Size special_size) {
  PageHeader p = (PageHeader)page;

  assert(page_size == BLCKSZ);
  assert(page_size > special_size + sizeof(PageHeaderData) - sizeof(ItemIdData));

  special_size = MAXALIGN(special_size);

  p->pd_lower = sizeof(PageHeaderData) - sizeof(ItemIdData);
  p->pd_upper = page_size - special_size;
  p->pd_special = page_size - special_size;
  PAGE_SET_PAGE_SIZE(page, page_size);
}

// Adds item to the given page.
//
// Note:
//  This does not assume that the item resides on a single page.
//  It is the responsiblity of the caller to act appropriately
//  depending on this fact.  The "pskip" routines provide a
//  friendlier interface, in this case.
//
//  This does change the status of any of the resources passed.
//  The semantics may change in the future.
//
//  This routine should probably be combined with others?
//
// Notes on interface:
//  If offsetNumber is valid, shuffle ItemId's down to make room
//  to use it, if PageManagerShuffle is true.  If PageManagerShuffle is
//  false, then overwrite the specified ItemId.  (PageManagerShuffle is
//  true by default, and is modified by calling PageManagerModeSet.)
//  If offsetNumber is not valid, then assign one by finding the first
//  one that is both unused and deallocated.
//
// NOTE: If offsetNumber is valid, and PageManagerShuffle is true, it
//  is assumed that there is room on the page to shuffle the ItemId's
//  down by one.
OffsetNumber page_add_item(Page page, Item item, Size size, OffsetNumber offset_num, ItemIdFlags flags) {
  int i;
  Size aligned_size;
  Offset lower;
  Offset upper;
  ItemId itemid;
  ItemId from_itemid;
  ItemId to_itemid;
  OffsetNumber limit;

  bool shuffled = false;

  // Find first unallocated offset_number.
  limit = OFFSET_NUMBER_NEXT(PAGE_GET_MAX_OFFSET_NUMBER(page));

  // Was offsetNumber passed in?
  if (OFFSET_NUMBER_IS_VALID(offset_num)) {
    if (PageManagerShuffle) {
      // Shuffle ItemId's (Do the PageManager Shuffle...).
      // TODO(gc): 为什么要shuffle
      for (i = (limit - 1); i >= offset_num; i--) {
        from_itemid = &((PageHeader)page)->pd_linp[i - 1];
        to_itemid = &((PageHeader)page)->pd_linp[i];
        *to_itemid = *from_itemid;
      }

      shuffled = true;  // Need to increase "lower".
    } else {
      itemid = &((PageHeader)page)->pd_linp[offset_num - 1];

      if (((*itemid).lp_flags & LP_USED) || ((*itemid).lp_len != 0)) {
        elog(ERROR, "%s: tried overwrite of used ItemId", __func__);

        return INVALID_OFFSET_NUMBER;
      }
    }
  } else {
    // OffsetNumber was not passed in, so find one.
    // Look for "recyclable" (unused & deallocated) ItemId.
    for (offset_num = 1; offset_num < limit; offset_num++) {
      itemid = &((PageHeader)page)->pd_linp[offset_num - 1];

      if ((((*itemid).lp_flags & LP_USED) == 0) && ((*itemid).lp_len == 0)) {
        break;
      }
    }
  }

  if (offset_num > limit) {
    lower = (Offset)(((char*)(&((PageHeader)page)->pd_linp[offset_num])) - ((char*)page));
  } else if (offset_num == limit || shuffled == true) {
    lower = ((PageHeader)page)->pd_lower + sizeof(ItemIdData);
  } else {
    lower = ((PageHeader)page)->pd_lower;
  }

  aligned_size = MAXALIGN(size);
  upper = ((PageHeader)page)->pd_upper - aligned_size;

  if (lower > upper) {
    return INVALID_OFFSET_NUMBER;
  }

  itemid = &((PageHeader)page)->pd_linp[offset_num - 1];
  (*itemid).lp_off = upper;
  (*itemid).lp_len = size;
  (*itemid).lp_flags = flags;
  memmove((char*)page + upper, item, size);
  ((PageHeader)page)->pd_lower = lower;
  ((PageHeader)page)->pd_upper = upper;

  return offset_num;
}

// Get a temporary page in local memory for special processing.
Page page_get_temp_page(Page page, Size special_size) {
  Size page_size;
  Size size;
  Page temp;
  PageHeader thdr;

  page_size = PAGE_GET_PAGE_SIZE(page);

  if ((temp = (Page)palloc(page_size)) == NULL) {
    elog(FATAL, "%s: cannot allocate %d bytes for temp page.", __func__, page_size);
  }

  thdr = (PageHeader)temp;

  // Copy old page in.
  memmove(temp, page, page_size);

  // Clear out the middle.
  size = (page_size - sizeof(PageHeaderData)) + sizeof(ItemIdData);
  size -= MAXALIGN(special_size);
  MemSet((char*)&(thdr->pd_linp[0]), 0, size);

  // Set high, low water marks.
  thdr->pd_lower = sizeof(PageHeaderData) - sizeof(ItemIdData);
  thdr->pd_upper = page_size - MAXALIGN(special_size);

  return temp;
}

// Copy temporary page back to permanent page after special processing
// and release the temporary page.
void page_restore_temp_page(Page temp_page, Page old_page) {
  Size page_size;

  page_size = PAGE_GET_PAGE_SIZE(temp_page);
  memmove((char*)old_page, (char*)temp_page, page_size);

  pfree(temp_page);
}

// itemid stuff for PageRepairFragmentation.
struct ItemIdSortData {
  int offset_index;  // linp array index.
  ItemIdData itemid_data;
};

static int itemid_compare(const void* itemid_p1, const void* itemid_p2) {
  if (((struct ItemIdSortData*)itemid_p1)->itemid_data.lp_off ==
      ((struct ItemIdSortData*)itemid_p2)->itemid_data.lp_off) {
    return 0;
  } else if (((struct ItemIdSortData*)itemid_p1)->itemid_data.lp_off <
             ((struct ItemIdSortData*)itemid_p2)->itemid_data.lp_off) {
    return 1;
  } else {
    return -1;
  }
}

// Frees fragmented space on a page.
void page_repair_fragmentation(Page page) {
  int i;
  struct ItemIdSortData* itemid_base;
  struct ItemIdSortData* itemid_ptr;
  ItemId lp;
  int nline;
  int nused;
  Offset upper;
  Size aligned_size;

  nline = (int16)PAGE_GET_MAX_OFFSET_NUMBER(page);
  nused = 0;

  for (i = 0; i < nline; i++) {
    lp = ((PageHeader)page)->pd_linp + i;

    if ((*lp).lp_flags & LP_USED) {
      nused++;
    }
  }

  if (nused == 0) {
    for (i = 0; i < nline; i++) {
      lp = ((PageHeader)page)->pd_linp + i;

      // Unused, but allocated
      // Indicate unused & deallocated.
      if ((*lp).lp_len > 0) {
        (*lp).lp_len = 0;
      }
    }

    ((PageHeader)page)->pd_upper = ((PageHeader)page)->pd_special;
  } else {
    itemid_base = (struct ItemIdSortData*)palloc(sizeof(struct ItemIdSortData) * nused);
    MemSet((char*)itemid_base, 0, sizeof(struct ItemIdSortData) * nused);
    itemid_ptr = itemid_base;

    for (i = 0; i < nline; i++) {
      lp = ((PageHeader)page)->pd_linp + i;

      if ((*lp).lp_flags & LP_USED) {
        itemid_ptr->offset_index = i;
        itemid_ptr->itemid_data = *lp;
        itemid_ptr++;
      } else {
        // Unused, but allocated
        // Indicate unused & deallocated.
        if ((*lp).lp_len > 0) {
          (*lp).lp_len = 0;
        }
      }
    }

    // Sort ItemIdSortData array.
    qsort((char*)itemid_base, nused, sizeof(struct ItemIdSortData), itemid_compare);

    // Compactify page.
    ((PageHeader)page)->pd_upper = ((PageHeader)page)->pd_special;

    for (i = 0, itemid_ptr = itemid_base; i < nused; i++, itemid_ptr++) {
      lp = ((PageHeader)page)->pd_linp + itemid_ptr->offset_index;
      aligned_size = MAXALIGN((*lp).lp_len);
      upper = ((PageHeader)page)->pd_upper - aligned_size;
      memmove((char*)page + upper, (char*)page + (*lp).lp_off, (*lp).lp_len);
      (*lp).lp_off = upper;
      ((PageHeader)page)->pd_upper = upper;
    }

    pfree(itemid_base);
  }
}

// Returns the size of the free (allocatable) space on a page.
Size page_get_free_space(Page page) {
  Size space;

  space = ((PageHeader)page)->pd_upper - ((PageHeader)page)->pd_lower;

  if (space < sizeof(ItemIdData)) {
    return 0;
  }

  space -= sizeof(ItemIdData);  // XXX not always true.

  return space;
}

// Sets mode to either: ShufflePageManagerMode (the default) or
// OverwritePageManagerMode. For use by access methods code
// for determining semantics of PageAddItem when the offsetNumber
// argument is passed in.
void page_manager_mode_set(PageManagerMode mode) {
  if (mode == ShufflePageManagerMode) {
    PageManagerShuffle = true;
  } else if (mode == OverwritePageManagerMode) {
    PageManagerShuffle = false;
  }
}

// This routine does the work of removing a tuple from an index page.
void page_index_tuple_delete(Page page, OffsetNumber offset_num) {
  PageHeader phdr;
  char* addr;
  ItemId tup;
  Size size;
  char* locn;
  int nbytes;
  int offidx;

  phdr = (PageHeader)page;

  // Change offset number to offset index.
  offidx = offset_num - 1;

  tup = PAGE_GET_ITEM_ID(page, offset_num);
  size = ITEM_ID_GET_LENGTH(tup);
  size = MAXALIGN(size);

  // Location of deleted tuple data.
  locn = (char*)(page + ITEM_ID_GET_OFFSET(tup));

  // First, we want to get rid of the pd_linp entry for the index tuple.
  // We copy all subsequent linp's back one slot in the array.
  nbytes = phdr->pd_lower - ((char*)&phdr->pd_linp[offidx + 1] - (char*)phdr);
  memmove((char*)&(phdr->pd_linp[offidx]), (char*)&(phdr->pd_linp[offidx + 1]), nbytes);

  // Now move everything between the old upper bound (beginning of tuple
  // space) and the beginning of the deleted tuple forward, so that
  // space in the middle of the page is left free.  If we've just
  // deleted the tuple at the beginning of tuple space, then there's no
  // need to do the copy (and bcopy on some architectures SEGV's if
  // asked to move zero bytes).

  // Beginning of tuple space.
  addr = (char*)(page + phdr->pd_upper);

  if (locn != addr) {
    memmove(addr + size, addr, (int)(locn - addr));
  }

  // Adjust free space boundary pointers.
  phdr->pd_upper += size;
  phdr->pd_lower -= sizeof(ItemIdData);

  // Finally, we need to adjust the linp entries that remain.
  if (!PAGE_IS_EMPTY(page)) {
    page_index_tuple_delete_adjust_line_pointers(phdr, locn, size);
  }
}

// Once the line pointers and tuple data have been shifted around
// on the page, we need to go down the line pointer vector and
// adjust pointers to reflect new locations.  Anything that used
// to be before the deleted tuple's data was moved forward by the
// size of the deleted tuple.
//
// This routine does the work of adjusting the line pointers.
// Location is where the tuple data used to lie; size is how
// much space it occupied.  We assume that size has been aligned
// as required by the time we get here.
//
// This routine should never be called on an empty page.
static void page_index_tuple_delete_adjust_line_pointers(PageHeader phdr, char* location, Size size) {
  int i;
  unsigned offset;

  // Location is an index into the page...
  offset = (unsigned)(location - (char*)phdr);

  for (i = PAGE_GET_MAX_OFFSET_NUMBER((Page)phdr) - 1; i >= 0; i--) {
    if (phdr->pd_linp[i].lp_off <= offset) {
      phdr->pd_linp[i].lp_off += size;
    }
  }
}
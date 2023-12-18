// =========================================================================
//
// page.h
//  POSTGRES buffer page abstraction definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: page.h,v 1.7 2000/01/26 05:58:33 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_PAGE_H_
#define RDBMS_STORAGE_PAGE_H_

typedef Pointer Page;

#define PAGE_IS_VALID(page) POINTER_IS_VALID(page)

#endif  // RDBMS_STORAGE_PAGE_H_

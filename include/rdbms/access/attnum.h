// =========================================================================
//
// attnum.h
//  POSTGRES attribute number definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: attnum.h,v 1.11 2000/01/26 05:57:49 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_ACCESS_ATTNUM_H_
#define RDBMS_ACCESS_ATTNUM_H_

#include <assert.h>
#include <stdbool.h>

#include "rdbms/postgres.h"

// User defined attribute numbers start at 1.   -ay 2/95
typedef int16 AttrNumber;

#define INVALID_ATTR_NUMBER                           0
#define ATTRIBUTE_NUMBER_IS_VALID(att_num)            ((bool)((att_num) != INVALID_ATTR_NUMBER))
#define ATTR_NUMBER_IS_FOR_USER_DEFINED_ATTR(att_num) ((bool)((att_num) > 0))
#define ATTR_NUMBER_GET_ATTR_OFFSET(att_num)          (assert(ATTR_NUMBER_IS_FOR_USER_DEFINED_ATTR(att_num)), ((att_num)-1))
#define ATTR_OFFSET_GET_ATTR_NUMBER(att_offset)       ((AttrNumber)(1 + att_offset))

#endif  // RDBMS_ACCESS_ATTNUM_H_

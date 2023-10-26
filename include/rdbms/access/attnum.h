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

#include "rdbms/c.h"

// User defined attribute numbers start at 1.   -ay 2/95
typedef int16 AttrNumber;

#define InvalidAttrNumber                               0
#define AttributeNumberIsValid(attributeNumber)         ((bool)((attributeNumber) != InvalidAttrNumber))
#define AttrNumberIsForUserDefinedAttr(attributeNumber) ((bool)((attributeNumber) > 0))
#define AttrNumberGetAttrOffset(att_num)                (assert(AttrNumberIsForUserDefinedAttr(att_num)), ((att_num)-1))
#define AttrOffsetGetAttrNumber(attribute_offset)       ((AttrNumber)(1 + attribute_offset))

#endif  // RDBMS_ACCESS_ATTNUM_H_

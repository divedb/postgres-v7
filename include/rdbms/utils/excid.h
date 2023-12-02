//===----------------------------------------------------------------------===//
//
// excid.h
//  POSTGRES known exception identifier definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: excid.h,v 1.10 2001/03/23 18:26:01 tgl Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_EXCID_H_
#define RDBMS_UTILS_EXCID_H_

extern Exception FailedAssertion;
extern Exception BadState;
extern Exception BadArg;
extern Exception Unimplemented;

extern Exception CatalogFailure;  // XXX inconsistent naming style.
extern Exception InternalError;   // XXX inconsistent naming style.
extern Exception SemanticError;   // XXX inconsistent naming style.
extern Exception SystemError;     // XXX inconsistent naming style.

#endif  // RDBMS_UTILS_EXCID_H_

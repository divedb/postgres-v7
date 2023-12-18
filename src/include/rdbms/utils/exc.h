//===----------------------------------------------------------------------===//
//
// exc.h
//  POSTGRES exception handling definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: exc.h,v 1.19 2001/02/10 02:31:29 tgl Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_EXC_H_
#define RDBMS_UTILS_EXC_H_

#include <setjmp.h>

#include "rdbms/postgres.h"

// 全局的可执行文件名
extern char* ExcFileName;
extern Index ExcLineNumber;

typedef sigjmp_buf ExcContext;

typedef Exception* ExcId;
typedef long ExcDetail;
typedef char* ExcData;

typedef struct ExcFrame {
  struct ExcFrame* link;
  ExcContext context;
  ExcId id;
  ExcDetail detail;
  ExcData data;
  ExcMessage message;
} ExcFrame;

extern ExcFrame* ExcCurFrameP;

#define EXC_BEGIN()                             \
  do {                                          \
    ExcFrame exception;                         \
    �                                         \
                                                \
        exception.link = ExcCurFrameP;          \
    if (sigsetjmp(exception.context, 1) == 0) { \
      ExcCurFrameP = &exception;

#define EXC_EXCEPT()             \
  }                              \
  ExcCurFrameP = exception.link; \
  }                              \
  else {                         \
    {
#define EXC_END() \
  }               \
  }               \
  }               \
  while (0)

#define RAISE4(x, t, d, message) exc_raise(&(x), (ExcDetail)(t), (ExcData)(d), (ExcMessage)(message))
#define RERAISE()                RAISE4(*exception.id, exception.detail, exception.data, exception.message)
typedef void ExcProc(Exception*, ExcDetail, ExcData, ExcMessage);

// In in exc.c
void enable_exception_handling(bool on);
void exc_raise(Exception* excp, ExcDetail detail, ExcData data, ExcMessage message);

// In excabort.c
void exc_abort(const Exception* excp, ExcDetail detail, ExcData data, ExcMessage message);

#endif  // RDBMS_UTILS_EXC_H_

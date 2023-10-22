#ifndef RDBMS_UTILS_EXC_H_
#define RDBMS_UTILS_EXC_H_

#include <setjmp.h>

#include "rdbms/config.h"

// 全局的可执行文件名
extern char* ExcFileName;
extern Index ExcLineNumber;

typedef sigjmp_buf ExcContext;

// ================================================
// Section 7: exception handling definitions
//            Assert, Trap, etc macros
// ================================================

typedef char* ExcMessage;
typedef struct Exception {
  ExcMessage message;
} Exception;

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

#define raise4(x, t, d, message) ExcRaise(&(x), (ExcDetail)(t), (ExcData)(d), (ExcMessage)(message))
#define reraise()                raise4(*exception.id, exception.detail, exception.data, exception.message)
typedef void ExcProc(Exception*, ExcDetail, ExcData, ExcMessage);

void EnableExceptionHandling(bool on);
void ExcRaise(Exception* excp, ExcDetail detail, ExcData data, ExcMessage message);

#endif  // RDBMS_UTILS_EXC_H_

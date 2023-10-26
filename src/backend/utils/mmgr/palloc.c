#include "rdbms/utils/palloc.h"

#include <string.h>

#include "rdbms/utils/mctx.h"

char* pstrdup(const char* string) {
  char* nstr;
  int len;

  nstr = palloc(len = strlen(string) + 1);
  memcpy(nstr, string, len);

  return nstr;
}

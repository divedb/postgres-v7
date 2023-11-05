#include "rdbms/utils/palloc.h"

#include <string.h>

char* pstrdup(const char* string) {
  char* nstr;
  int len;

  nstr = palloc(len = strlen(string) + 1);
  memcpy(nstr, string, len);

  return nstr;
}

#include "rdbms/miscadmin.h"

// Note:
//  Be very careful with the order of calls in the InitPostgres function.
void init_postgres(const char* dbname) {
  bool bootstrap = IS_BOOTSTRAP_PROCESSING_MODE();

  be_portal_init();
}
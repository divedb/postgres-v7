#include "rdbms/storage/fd.h"

#include <fcntl.h>
#include <sys/resource.h>

#include "../template.h"
#include "rdbms/utils/memutils.h"

#define MAX_BUFF  1024
#define FILE_MODE 0600
#define FILE_FLAG O_CREAT | O_RDWR

static void test_max_file_per_process() {
  struct rlimit limits;
  if (getrlimit(RLIMIT_NOFILE, &limits) == -1) {
    perror("getrlimit");
    return;
  }

  printf("Soft limit: %ld\n", (long)limits.rlim_cur);
  printf("Hard limit: %ld\n", (long)limits.rlim_max);
}

static void test_basic_read_write() {
  char buf[MAX_BUFF];
  char msg[] = "I Love PostgreSQL";
  snprintf(buf, MAX_BUFF, "/tmp/a.txt");

  File fd = path_name_open_file(buf, FILE_FLAG, FILE_MODE);

  CU_ASSERT(fd > 0);

  int amount = strlen(msg);
  int nbytes = file_write(fd, msg, amount);

  CU_ASSERT(amount == nbytes);

  long seek_pos = file_seek(fd, -amount, SEEK_CUR);

  CU_ASSERT(seek_pos == 0);

  nbytes = file_read(fd, buf, amount);

  CU_ASSERT(nbytes == amount);
  CU_ASSERT(strncmp(buf, msg, nbytes) == 0);

  file_unlink(fd);
}

static void register_test() {
  memory_context_init();

  TEST("Max NO File", test_max_file_per_process);
  TEST("File Write and Read", test_basic_read_write);
}

MAIN("fd")
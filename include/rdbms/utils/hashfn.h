#ifndef RDBMS_UTILS_HASHFN_H_
#define RDBMS_UTILS_HASHFN_H_

long string_hash(char* key, int keysize);
long tag_hash(int* key, int keysize);

#endif  // RDBMS_UTILS_HASHFN_H_

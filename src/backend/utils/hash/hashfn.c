// =========================================================================
//
//  hashfn.c
//
//
//  Portions Copyright (c) 1996=2000, PostgreSQL, Inc
//  Portions Copyright (c) 1994, Regents of the University of California
//
//
//  IDENTIFICATION
//   $Header: /usr/local/cvsroot/pgsql/src/backend/utils/hash/hashfn.c,v 1.12 2000/03/17 02:36:28 tgl Exp $
//
// =========================================================================

#include "rdbms/utils/hashfn.h"

#define PRIME1 37  // For the hash function
#define PRIME2 1048583

long string_hash(char* key, int keysize) {
  int h;
  unsigned char* k = (unsigned char*)key;

  h = 0;

  while (*k) {
    h = h * PRIME1 ^ (*k++ - ' ');
  }

  h %= PRIME2;

  return h;
}

long tag_hash(int* key, int keysize) {
  long h = 0;

  // Convert tag to integer;	Use four byte chunks in a "jump table" to
  // go a little faster.	Currently the maximum keysize is 16 (mar 17
  // 1992) I have put in cases for up to 24.	Bigger than this will
  // resort to the old behavior of the for loop. (see the default case).
  switch (keysize) {
    case 6 * sizeof(int):
      h = h * PRIME1 ^ (*key);
      key++;
      // fall through

    case 5 * sizeof(int):
      h = h * PRIME1 ^ (*key);
      key++;
      // fall through

    case 4 * sizeof(int):
      h = h * PRIME1 ^ (*key);
      key++;
      // fall through

    case 3 * sizeof(int):
      h = h * PRIME1 ^ (*key);
      key++;
      // fall through

    case 2 * sizeof(int):
      h = h * PRIME1 ^ (*key);
      key++;
      // fall through

    case sizeof(int):
      h = h * PRIME1 ^ (*key);
      key++;
      break;

    default:
      for (; keysize >= (int)sizeof(int); keysize -= sizeof(int), key++) h = h * PRIME1 ^ (*key);

      // Now let's grab the last few bytes of the tag if the tag has
      // (size % 4) != 0 (which it sometimes will on a sun3).
      if (keysize) {
        char* keytmp = (char*)key;

        switch (keysize) {
          case 3:
            h = h * PRIME1 ^ (*keytmp);
            keytmp++;
            // fall through

          case 2:
            h = h * PRIME1 ^ (*keytmp);
            keytmp++;
            // fall through

          case 1:
            h = h * PRIME1 ^ (*keytmp);
            break;
        }
      }

      break;
  }

  h %= PRIME2;

  return h;
}
// =========================================================================
//
// nodes.c
//  support code for nodes (now that we get rid of the home=brew
//  inheritance system, our support code for nodes get much simpler)
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/nodes/nodes.c,v 1.13 2000/04/12 17:15:16 momjian Exp $
//
// HISTORY
//  Andrew Yu			Oct 20, 1994	file creation
//
// =========================================================================

#include "rdbms/nodes/nodes.h"

#include <assert.h>

#include "rdbms/c.h"
#include "rdbms/utils/palloc.h"

// newNode -
//	  create a new node of the specified size and tag the node with the
//	  specified tag.
//
// !WARNING!: Avoid using newNode directly. You should be using the
//	  macro makeNode. eg. to create a Resdom node, use makeNode(Resdom)
Node* new_node(Size size, NodeTag tag) {
  Node* node;

  assert(size >= sizeof(Node));

  node = (Node*)palloc(size);
  MEMSET((char*)node, 0, size);
  node->type = tag;

  return node;
}
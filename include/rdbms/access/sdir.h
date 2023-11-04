// =========================================================================
//
// sdir.h
//  POSTGRES scan direction definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: sdir.h,v 1.7 2000/01/26 05:57:51 momjian Exp $
//
// =========================================================================
///

#ifndef RDBMS_ACCESS_SDIR_H_
#define RDBMS_ACCESS_SDIR_H_

// ScanDirection was an int8 for no apparent reason. I kept the original
// values because I'm not sure if I'll break anything otherwise.  -ay 2/95
typedef enum ScanDirection {
  BackwardScanDirection = -1,
  NoMovementScanDirection = 0,
  ForwardScanDirection = 1
} ScanDirection;

#define SCAN_DIRECTION_IS_VALID(direction) \
  ((bool)(BackwardScanDirection <= direction && direction <= ForwardScanDirection))

#define SCAN_DIRECTION_IS_BACKWARD(direction)    ((bool)(direction == BackwardScanDirection))
#define SCAN_DIRECTION_IS_NO_MOVEMENT(direction) ((bool)(direction == NoMovementScanDirection))
#define SCAN_DIRECTION_IS_FORWARD(direction)     ((bool)(direction == ForwardScanDirection))

#endif  // RDBMS_ACCESS_SDIR_H_

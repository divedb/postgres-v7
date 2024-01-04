//===----------------------------------------------------------------------===//
//
// strat.h
//  index strategy type definitions
//  (separated out from original istrat.h to avoid circular refs)
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: strat.h,v 1.21 2001/03/22 06:16:20 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_ACCESS_STRAT_H_
#define RDBMS_ACCESS_STRAT_H_

#include "rdbms/access/skey.h"

typedef uint16 StrategyNumber;

#define INVALID_STRATEGY 0

typedef struct StrategyTransformMapData {
  StrategyNumber strategy[1];  // VARIABLE LENGTH ARRAY.
} StrategyTransformMapData;

typedef StrategyTransformMapData* StrategyTransformMap;

typedef struct StrategyOperatorData {
  StrategyNumber strategy;
  bits16 flags;  // Scan qualification flags h/skey.h.
} StrategyOperatorData;

typedef StrategyOperatorData* StrategyOperator;

// Conjunctive term.
typedef struct StrategyTermData {
  uint16 degree;
  StrategyOperatorData operatorData[1];  // VARIABLE LENGTH.
} StrategyTermData;

typedef StrategyTermData* StrategyTerm;

// Disjunctive normal form.
typedef struct StrategyExpressionData {
  StrategyTerm term[1];    // VARIABLE LENGTH ARRAY.
} StrategyExpressionData;  // VARIABLE LENGTH STRUCTURE.

typedef StrategyExpressionData* StrategyExpression;

typedef struct StrategyEvaluationData {
  StrategyNumber max_strategy;
  StrategyTransformMap negate_transform;
  StrategyTransformMap commute_transform;
  StrategyTransformMap negate_commute_transform;
  StrategyExpression expression[12];  // XXX VARIABLE LENGTH.
} StrategyEvaluationData;

typedef StrategyEvaluationData* StrategyEvaluation;

#define STRATEGY_TRANSFORM_MAP_IS_VALID(transform) POINTER_IS_VALID(transform)

#define AM_STEATEGIES(foo) foo

typedef struct StrategyMapData {
  ScanKeyData entry[1];  // VARIABLE LENGTH ARRAY.
} StrategyMapData;

typedef StrategyMapData* StrategyMap;

typedef struct IndexStrategyData {
  StrategyMapData strategyMapData[1];  // VARIABLE LENGTH ARRAY.
} IndexStrategyData;

typedef IndexStrategyData* IndexStrategy;

#endif  // RDBMS_ACCESS_STRAT_H_

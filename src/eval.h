#ifndef EVAL_H_
#define EVAL_H_

#include "env.h"
#include "value.h"

value* value_evaluate(value* t, environment* env);

void environment_register_builtins(environment* e);

#endif  // EVAL_H_

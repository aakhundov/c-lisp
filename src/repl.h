#ifndef REPL_H_
#define REPL_H_

#include "env.h"
#include "parse.h"

void run_repl(parser* p, environment* env);

#endif  // REPL_H_

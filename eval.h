#ifndef EVAL__H_
#define EVAL__H_

#include "position.h"

#define INFINITI 1000
#define NEG_INFINITI -1000
#define WHITE_WIN INFINITI
#define BLACK_WIN NEG_INFINITI

extern int eval(const struct position *restrict const pos);

#endif // EVAL__H_

#ifndef PERFT__H_
#define PERFT__H_

#include <stdint.h>
#include "move.h"
#include "position.h"

uint64_t perft_test(const struct position *restrict pos, int depth);

#endif // PERFT__H_

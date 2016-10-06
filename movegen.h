#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include "move.h"

struct position;
struct savepos;
struct savepos;

extern int attacks(const struct position * const restrict pos, uint8_t side, int square);
extern int in_check(const struct position * const restrict pos, uint8_t side);

#endif // MOVEGEN__H_

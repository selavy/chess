#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include "general.h"
#include "move.h"
#include "position.h"

extern int attacks(const struct position * const restrict pos, uint8_t side, int square);
extern int in_check(const struct position * const restrict pos, uint8_t side);
extern uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves);
extern uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves);

#endif // MOVEGEN__H_

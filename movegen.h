#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include "move.h"

struct position;
struct savepos;

extern void make_move(struct position * restrict p, move m, struct savepos * restrict sp);
extern void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp);
extern int attacks(const struct position * const restrict pos, uint8_t side, int square);
extern int in_check(const struct position * const restrict pos, uint8_t side);
extern uint32_t generate_moves(const struct position * const restrict pos, move * restrict moves);

#endif // MOVEGEN__H_
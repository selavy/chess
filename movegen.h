#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include "move.h"

struct position;
struct savepos;
struct saveposex;

extern void make_move(struct position *restrict p, move m, struct saveposex *restrict sp);
extern void test_make_move(int argc, char **argv);

extern void undo_move(struct position * restrict p, move m, const struct saveposex * restrict sp);
extern void test_undo_move(int argc, char **argv);

extern int attacks(const struct position * const restrict pos, uint8_t side, int square);
extern int in_check(const struct position * const restrict pos, uint8_t side);
extern uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves);

#endif // MOVEGEN__H_

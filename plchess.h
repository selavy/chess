#ifndef PLCHESS__H_
#define PLCHESS__H_

#include "types.h"
#include "move.h"

// --- Interface Functions ---

// TODO: move this out of interface
extern uint32_t gen_legal_moves(const struct position *const restrict pos, move *restrict moves);

// TODO: move this out of interface
extern void make_move(struct position *restrict p, move m, struct savepos *restrict sp);

// TODO: move this out of interface
extern void undo_move(struct position * restrict p, move m, const struct savepos * restrict sp);

// TODO: move this out of interface
extern uint32_t generate_moves(const struct position *const restrict pos, move *restrict moves);

// --- Test Functions ---

extern void test_perft(int argc, char **argv);

extern void test_deep_perft(int argc, char **argv);

extern void test_make_move(int argc, char **argv);

extern void test_undo_move(int argc, char **argv);

#endif // PLCHESS_H_

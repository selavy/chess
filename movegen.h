#ifndef MOVEGEN__H_
#define MOVEGEN__H_

#include <stdint.h>
#include "move.h"
#include "position.h"

extern int is_legal_ex(const struct position *const restrict pos, uint64_t pinned, move m);
extern uint64_t generate_checkers(const struct position *const restrict pos, uint8_t side);
extern uint64_t generate_attacked(const struct position *const restrict pos, const uint8_t side);
extern move *generate_evasions(const struct position *const restrict pos, uint64_t checkers, move *restrict moves);
extern uint64_t pinned_pieces(const struct position *const restrict pos, uint8_t side, uint8_t kingcolor);
extern int attacks(const struct position *const restrict pos, uint8_t side, int square);
extern int generate_legal_moves(const struct position *const restrict pos, move *restrict moves);
extern int in_check(const struct position * const restrict pos, uint8_t side);
extern int legal_move(const struct position *const restrict pos, move m);
extern move *generate_non_evasions(const struct position *const restrict pos, move *restrict moves);

#endif // MOVEGEN__H_

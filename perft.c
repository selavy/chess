#include "perft.h"
#include <assert.h>
#include "types.h"
#include "movegen.h"

uint64_t checks     = 0;
uint64_t captures   = 0;
uint64_t enpassants = 0;
uint64_t castles    = 0;
uint64_t promotions = 0;
uint64_t checkmates = 0;

uint64_t perft(int depth, struct position * const restrict pos, move pmove, int ply) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct savepos sp;
    move moves[MAX_MOVES];
    
    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    if (depth == 0) {
//#define COUNTERS
#ifdef COUNTERS        
        if (pmove != 0) {
            if (in_check(pos, pos->wtm) != 0) {
                ++checks;
            }
            if (is_castle(pmove) != 0) {
                ++castles;
            }
            if (CAPTURE(pmove) != NO_CAPTURE) {
                ++captures;
                if (ENPASSANT(pmove) != 0) {
                    ++enpassants;
                }
            }
            if (PROMOTE(pmove) != NO_PROMOTION) {
                ++promotions;
            }
        }
#endif // ~COUNTERS
        return 1;
    }
    
    nmoves = generate_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        nodes += perft(depth - 1, pos, moves[i], ply + 1);
        undo_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
    }

    return nodes;
}

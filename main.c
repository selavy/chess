#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include "types.h"
#include "move.h"
#include "movegen.h"
#include "read_fen.h"

static uint64_t checks = 0;
static uint64_t captures = 0;
static uint64_t enpassants = 0;
static uint64_t castles = 0;
static uint64_t promotions = 0;
static uint64_t checkmates = 0;

uint64_t perft_ex(int depth, struct position * const restrict pos, move pmove, int ply) {
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
        nodes += perft_ex(depth - 1, pos, moves[i], ply + 1);
        undo_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
    }

    return nodes;
}
uint64_t perft(int depth) {
    static struct position pos;
    set_initial_position(&pos);
    return perft_ex(depth, &pos, 0, 0);
}

int main(int argc, char **argv) {
    int depth;
    uint64_t nodes;
    static struct position pos;

#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif

    #define DEPTH 8
    
    // starting position:
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

    // kiwi pete position:
    /* const char *fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"; */

    // position #3
    //const char *fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";

    // position #4
    //const char *fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";

    // position #5
    //const char* fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -";

    // position #6
    //const char* fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -";
    
    printf("Perft:\n");
    if (read_fen(&pos, fen) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        exit(EXIT_FAILURE);
    }

    for (depth = DEPTH; depth < DEPTH+1; ++depth) {
        checks = 0;
        captures = 0;
        enpassants = 0;
        castles = 0;
        checkmates = 0;
        promotions = 0;
        
        nodes = perft_ex(depth, &pos, 0, 0);
        printf("Perft(%u): Nodes=%" PRIu64 ", Captures=%" PRIu64 ", E.p.=%" PRIu64
                ", Castles=%" PRIu64 ", Promotions=%" PRIu64
                ", Checks=%" PRIu64 ", Checkmates=%" PRIu64 "\n",
                depth, nodes, captures, enpassants, castles, promotions,
                checks, checkmates);
    }

    return 0;
}

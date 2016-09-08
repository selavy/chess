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
#include "perft.h"

int main(int argc, char **argv) {
    if (perft_count_test() != 0) {
        fputs("FAILURE!!\n", stderr);
    } else {
        fputs("Success.\n", stdout);
    }
    
    #if 0
    int depth;
    /* uint64_t nodes; */
    static struct position pos;

#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif

    #define DEPTH 7
    
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

    for (depth = 0; depth < DEPTH+1; ++depth) {
        /* reset_counts(); */
        
        /* nodes = perft(depth, &pos, 0); */
        /* printf("Perft(%u): Nodes=%" PRIu64 ", Captures=%" PRIu64 ", E.p.=%" PRIu64 */
        /*         ", Castles=%" PRIu64 ", Promotions=%" PRIu64 */
        /*         ", Checks=%" PRIu64 ", Checkmates=%" PRIu64 "\n", */
        /*         depth, nodes, captures, enpassants, castles, promotions, */
        /*         checks, checkmates); */

        printf("Perft(%u): %" PRIu64 "\n",
               depth, perft_bulk(depth, &pos));
    }
    #endif

    return 0;
}

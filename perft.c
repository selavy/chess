#include "perft.h"
#include <stdio.h>
#include <assert.h>
#include "types.h"
#include "movegen.h"
#include "read_fen.h"

uint64_t checks     = 0;
uint64_t captures   = 0;
uint64_t enpassants = 0;
uint64_t castles    = 0;
uint64_t promotions = 0;
uint64_t checkmates = 0;

void reset_counts() {
    checks     = 0;
    captures   = 0;
    enpassants = 0;
    castles    = 0;
    promotions = 0;
    checkmates = 0;
}

uint64_t perft(int depth, struct position * const restrict pos, move pmove) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct savepos sp;
    move moves[MAX_MOVES];
    
    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    if (depth == 0) {
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
        return 1;
    }
    
    nmoves = generate_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        nodes += perft(depth - 1, pos, moves[i]);
        undo_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
    }

    return nodes;
}


uint64_t perft_bulk(int depth, struct position * const restrict pos) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct savepos sp;
    move moves[MAX_MOVES];

    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    nmoves = generate_moves(pos, &moves[0]);

    if (depth > 0) {
        for (i = 0; i < nmoves; ++i) {
            make_move(pos, moves[i], &sp);
            assert(validate_position(pos) == 0);
            nodes += perft(depth - 1, pos, moves[i]);
            undo_move(pos, moves[i], &sp);
            assert(validate_position(pos) == 0);
        }
    }

    return nodes;
}

struct expected_t {
    uint64_t nodes;
    uint64_t captures;    
    uint64_t enpassants;
    uint64_t castles;    
    uint64_t promotions;    
    uint64_t checks;    
    uint64_t checkmates;
};

int perft_count_test() {
    int depth;
    uint64_t nodes;
    struct position pos;
    const struct expected_t *e;
    #define max_depth 7

    const struct expected_t expected[max_depth] = {
        { .nodes=1        , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=20       , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=400      , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=8902     , .captures=34     , .enpassants=0   , .castles=0, .promotions=0, .checks=12    , .checkmates=0 },
        { .nodes=197281   , .captures=1576   , .enpassants=0   , .castles=0, .promotions=0, .checks=469   , .checkmates=0 },
        { .nodes=4865609  , .captures=82719  , .enpassants=258 , .castles=0, .promotions=0, .checks=27351 , .checkmates=0 },
        { .nodes=119060324, .captures=2812008, .enpassants=5248, .castles=0, .promotions=0, .checks=809099, .checkmates=0 },        
    };
    
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }
    
    for (depth = 0; depth < max_depth; ++depth) {
        printf("Beginning depth %d...", depth);
        reset_counts();
        nodes = perft(depth, &pos, 0);
        e = &expected[depth];
        if (nodes != e->nodes) {
            printf("Failed nodes!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->nodes, nodes);            
            return 1;
        } else if (captures != e->captures) {
            printf("Failed captures!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->captures, captures);
            return 2;
        } else if (enpassants != e->enpassants) {
            printf("Failed enpassants!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->enpassants, enpassants);            
            return 3;
        } else if (castles != e->castles) {
            printf("Failed castles!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->castles, castles);            
            return 4;
        } else if (promotions != e->promotions) {
            printf("Failed promotions!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->promotions, promotions);            
            return 5;
        } else if (checks != e->checks) {
            printf("Failed checks!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->checks, checks);            
            return 6;
        } else if (checkmates != e->checkmates) {
            printf("Failed checkmates!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   e->checkmates, checkmates);            
            return 7;
        } else {
            printf("Passed.\n");
        }
    }

    return 0;
}

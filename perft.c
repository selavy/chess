#include "perft.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
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

uint64_t perft_ex(int depth, struct position *const restrict pos, smove_t pmove, int cap) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct saveposex sp;
    smove_t moves[MAX_MOVES];
    struct position tmp;
    
    if (in_check(pos, FLIP(pos->wtm))) {
        return 0;
    }
    if (depth == 0) {
        if (pmove != 0) {
            if (in_check(pos, pos->wtm) != 0) {
                ++checks;
            }
            if (cap != EMPTY) {
                ++captures;
            }
            if (SM_FLAGS(pmove) == SM_CASTLE) {
                ++castles;
            } else if (SM_FLAGS(pmove) == SM_EP) {
                ++enpassants;
                ++captures; // e.p. don't set the sp->captured_pc flag
            } else if (SM_FLAGS(pmove) == SM_PROMO) {
                ++promotions;
            }
        }
        return 1;
    }
    
    memcpy(&tmp, pos, sizeof(tmp));
    nmoves = generate_moves_ex(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move_ex(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        nodes += perft_ex(depth - 1, pos, moves[i], sp.captured_pc);
        undo_move_ex(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
        //assert(memcmp(&tmp, pos, sizeof(tmp)) == 0);
        if (memcmp(&tmp, pos, sizeof(tmp)) != 0) {
            printf("Original position:\n");
            full_position_print(&tmp);
            printf("\n\nAfter undo move:\n");
            full_position_print(pos);
            printf("\nMove:\n");
            smove_print(moves[i]);
            assert(0);
        }
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

static int perft_bulk_test_ex(const char *name, const char *fen, const uint64_t *expected, int max_depth) {
    int depth;
    uint64_t nodes;
    struct position pos;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }

    printf("Running test for %s\n", name);
    for (depth = 0; depth < max_depth; ++depth) {
        printf("Beginning depth %d...", depth);
        reset_counts();
        nodes = perft(depth, &pos, 0);
        if (nodes != expected[depth]) {
            printf("Failed nodes!\n");
            printf("Expected = %" PRIu64 ", Actual = %" PRIu64 "\n",
                   expected[depth], nodes);
            return 1;
        } else {
            printf("Passed.\n");
        }
    }

    return 0;
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

static int perft_count_test_ex(const char *name, const char *fen, const struct expected_t *expected, int max_depth) {
    int depth;
    uint64_t nodes;
    struct position pos;
    const struct expected_t *e;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }

    printf("Running test for %s\n", name);
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

int perft_count_test() {
    int ret;
    
    const char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    #define start_position_depth 6
    const struct expected_t start_position_expected[start_position_depth] = {
        { .nodes=1        , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=20       , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=400      , .captures=0      , .enpassants=0   , .castles=0, .promotions=0, .checks=0     , .checkmates=0 },
        { .nodes=8902     , .captures=34     , .enpassants=0   , .castles=0, .promotions=0, .checks=12    , .checkmates=0 },
        { .nodes=197281   , .captures=1576   , .enpassants=0   , .castles=0, .promotions=0, .checks=469   , .checkmates=0 },
        { .nodes=4865609  , .captures=82719  , .enpassants=258 , .castles=0, .promotions=0, .checks=27351 , .checkmates=0 },
        /* { .nodes=119060324, .captures=2812008, .enpassants=5248, .castles=0, .promotions=0, .checks=809099, .checkmates=0 },         */
    };
    
    if ((ret = perft_count_test_ex("starting position", starting_position, &start_position_expected[0], start_position_depth)) != 0) {
        return ret;
    }

    const char *kiwi_pete_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    #define kiwi_depth 5
    const struct expected_t kiwi_expected[kiwi_depth] = {
        { .nodes=1, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },        
        { .nodes=48, .captures=8, .enpassants=0, .castles=2, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=2039, .captures=351, .enpassants=1, .castles=91, .promotions=0, .checks=3, .checkmates=0 },
        { .nodes=97862, .captures=17102, .enpassants=45, .castles=3162, .promotions=0, .checks=993, .checkmates=0 },
        { .nodes=4085603, .captures=757163, .enpassants=1929, .castles=128013, .promotions=15172, .checks=25523, .checkmates=0 },
        /* { .nodes=193690690, .captures=35043416, .enpassants=73365, .castles=4993637, .promotions=8392, .checks=3309887, .checkmates=0 }, */
    };
    if ((ret = perft_count_test_ex("kiwi pete", kiwi_pete_fen, &kiwi_expected[0], kiwi_depth)) != 0) {
        return ret;
    }

    const char *position3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    #define pos3_depth 5
    const struct expected_t pos3_exp[pos3_depth] = {
        { .nodes=1, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=14, .captures=1, .enpassants=0, .castles=0, .promotions=0, .checks=2, .checkmates=0 },
        { .nodes=191, .captures=14, .enpassants=0, .castles=0, .promotions=0, .checks=10, .checkmates=0 },
        { .nodes=2812, .captures=209, .enpassants=2, .castles=0, .promotions=0, .checks=267, .checkmates=0 },
        { .nodes=43238, .captures=3348, .enpassants=123, .castles=0, .promotions=0, .checks=1680, .checkmates=0 },                
    };
    if ((ret = perft_count_test_ex("position #3", position3, &pos3_exp[0], pos3_depth)) != 0) {
        return ret;
    }

    const char *pos4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";
    #define pos4_depth 5
    const struct expected_t pos4_exp[pos4_depth] = {
        { .nodes=1, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=6, .captures=0, .enpassants=0, .castles=0, .promotions=0, .checks=0, .checkmates=0 },
        { .nodes=264, .captures=87, .enpassants=0, .castles=6, .promotions=48, .checks=10, .checkmates=0 },
        { .nodes=9467, .captures=1021, .enpassants=4, .castles=0, .promotions=120, .checks=38, .checkmates=0 },
        { .nodes=422333, .captures=131393, .enpassants=0, .castles=7795, .promotions=60032, .checks=15492, .checkmates=0 },                        
    };
    if ((ret = perft_count_test_ex("position #4", pos4, &pos4_exp[0], pos4_depth)) != 0) {
        return ret;
    }

    const char *pos5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -";
    #define pos5_depth 5
    uint64_t pos5_exp[pos5_depth] = {
        1,
        44,
        1486,
        62379,
        2103487
    };
    if ((ret = perft_bulk_test_ex("position #5", pos5, &pos5_exp[0], pos5_depth)) != 0) {
        return ret;
    }

    const char* pos6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -";
    #define pos6_depth 5
    uint64_t pos6_exp[pos6_depth] = {
        1,
        46,
        2079,
        89890,
        3894594
    };
    if ((ret = perft_bulk_test_ex("position #6", pos6, &pos6_exp[0], pos6_depth)) != 0) {
        return ret;
    }    
    
    return 0;
}

void test_perft() {
    if (perft_count_test() != 0) {
        fputs("FAILURE!!\n", stderr);
    } else {
        fputs("Success.\n", stdout);
    }    
}

void test_new_perft() {
    const int max_depth = 7;
    int depth;
    uint64_t nodes;
    struct position pos;
    const char *start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    const char *name = "starting position";


    const char *fen = start_pos_fen;
    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return;
    }
    printf("Running test for %s\n", name);
    for (depth = 6; depth < max_depth; ++depth) {
        printf("Beginning depth %d...", depth);
        reset_counts();
        nodes = perft_ex(depth, &pos, 0, EMPTY);
        printf("Perft(%u): Nodes(%" PRIu64 ") Captures(%" PRIu64 ") E.p.(%" PRIu64 ") "
               "Castles(%" PRIu64 ") Promotions(%" PRIu64 ") Checks(%" PRIu64 ") "
               "Checkmates(%" PRIu64 ")\n",
               depth, nodes, captures, enpassants, castles, promotions, checks, checkmates);
    }
}

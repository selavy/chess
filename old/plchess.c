#define _GNU_SOURCE
#include "plchess.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "general.h"
#include "move.h"
#include "position.h"
#include "movegen.h"

enum xboard_state { IDLE, SETUP, PLAYING };

// --- Types ---
struct xboard_settings {
    int state;
    FILE *log;
};

// --- Static Function Prototypes ---
static int xboard_settings_init(struct xboard_settings *settings, const char *log);
static int xboard_settings_finalize(struct xboard_settings *settings);
static int handle_xboard_input(const char * const line, size_t bytes, struct xboard_settings *settings);
static void sighandler(int signum);
static const char * xboard_move_print(move mv);

// --- Interface Functions ---
void xboard_main() {
    FILE *istream;
    char *line = 0;
    size_t len = 0;
    ssize_t read = 0;
    struct xboard_settings settings;

    // TODO(plesslie): until I implement actual move selection algo
    srand(0);

    // xboard sends SIGINT when the opponent moves - can use to wake
    // up from pondering once that is implemented
    signal(SIGINT, &sighandler);

    if (xboard_settings_init(&settings, "/tmp/output.txt") != 0) {
	fprintf(stderr, "Failed to initialize xboard settings...\n");
	exit(EXIT_FAILURE);
    }

    istream = fdopen(STDIN_FILENO, "rb");
    if (istream == 0) {
	exit(EXIT_FAILURE);
    }
    // turn off i/o buffering
    setbuf(stdout , NULL);
    setbuf(istream, NULL);

    fprintf(settings.log, "Starting up myengine...\n");
    while ((read = getline(&line, &len, istream)) > 0) {
        line[read-1] = 0;
        if (handle_xboard_input(line, read-1, &settings) != 0) {
            break;
        }
    }

    printf("Bye.\n");
    free(line);
    fclose(istream);

    if (xboard_settings_finalize(&settings) != 0) {
	fprintf(stderr, "Failed to finalize xboard settings...\n");
    }
}

// --- Static Function Definitions ---
static int xboard_settings_init(struct xboard_settings *settings, const char *log) {
    settings->state = IDLE;
    if (log == 0) {
	log = "/tmp/output.txt";
    }
    settings->log = fopen(log, "w");
    if (!settings->log) {
	return 1;
    }
    setbuf(settings->log, NULL);
    return 0;
}

static int xboard_settings_finalize(struct xboard_settings *settings) {
    if (settings->log) {
	fclose(settings->log);
    }
    return 0;
}

#define XSTRNCMP(X,Y) strncmp(X, Y, strlen(Y))
static int handle_xboard_input(const char * const line, size_t bytes, struct xboard_settings *settings) {
    static struct position position;
    static move moves[MAX_MOVES];
    static struct savepos sp;

    fprintf(settings->log, "Received command: '%s'\n", line);
    if (settings->state == IDLE) {
	// TODO:
	settings->state = SETUP;
    }
    if (settings->state == SETUP) {
        if (strcmp(line, "xboard") == 0) {
	    set_initial_position(&position);
        } else if (XSTRNCMP(line, "protover") == 0) {
            if (line[9] != '2') {
                fprintf(stderr, "Unrecognized protocol version:\n");
                return 1;
            }
            printf("feature myname=\"experiment\"\n");
            printf("feature reuse=0\n");
            printf("feature analyze=0\n");
	    printf("feature time=0\n");
            printf("feature done=1\n");
        } else if (strcmp(line, "new") == 0) {
            // no-op
        } else if (strcmp(line, "random") == 0) {
            // no-op
        } else if (XSTRNCMP(line, "level") == 0) {
            // TODO(plesslie): parse time control
        } else if (strcmp(line, "post") == 0) {
            // TODO(plesslie):
            // turn on thinking/pondering output
            // thinking output should be in the format "ply score time nodes pv"
            // E.g. "9 156 1084 48000 Nf3 Nc6 Nc3 Nf6" ==> 
            // "9 ply, score=1.56, time = 10.84 seconds, nodes=48000, PV = "Nf3 Nc6 Nc3 Nf6""
        } else if (XSTRNCMP(line, "accepted") == 0) {
            // no-op
        } else if (strcmp(line, "hard") == 0) {
	    settings->state = PLAYING;
        } else if (strcmp(line, "white") == 0) {
            // TODO(plesslie): setup side
        } else if (strcmp(line, "black") == 0) {
            // TODO(plesslie): setup side
        } else if (XSTRNCMP(line, "time") == 0) {
            // no-op
        } else if (XSTRNCMP(line, "otim") == 0) {
            // no-op
        } else if (strcmp(line, "force") == 0) {
            // no-op
            // stop thinking about the current position
        } else {
            printf("Error (unknown command): %.*s\n", (int)bytes, line);
            return 1;
        }
    } else if (settings->state == PLAYING) {
	if (strcmp(line, "go") == 0) {
	    // no-op
	} else if (strcmp(line, "white") == 0) {
	    return 0;
	} else if (bytes == 4 || bytes == 5) { 	// read move from input
	    const uint32_t from = (line[1]-'1')*8 + (line[0]-'a');
	    const uint32_t to   = (line[3]-'1')*8 + (line[2]-'a');
	    fprintf(settings->log, "Parsed move as %u -> %u, %s -> %s\n",
		    to, from, sq_to_str[from], sq_to_str[to]);
	    move m;
	    if (bytes == 4) {
		m = SIMPLEMOVE(from, to);
	    } else { // promotion
		uint32_t prm;
		switch (line[4]) {
		case 'q': prm = MV_PRM_QUEEN ; break;
		case 'n': prm = MV_PRM_KNIGHT; break;
		case 'b': prm = MV_PRM_BISHOP; break;
		case 'r': prm = MV_PRM_ROOK  ; break;
		default:
		    printf("Invalid move: %s\n", line);
		    return 1;
		}
		m = MOVE(from, to, prm, MV_FALSE, MV_FALSE);
	    }
		    
	    fprintf(settings->log, "Position before:\n");
	    position_print(position.sqtopc, settings->log);
	    make_move(&position, m, &sp);
	    fprintf(settings->log, "Position after:\n");
	    position_print(position.sqtopc, settings->log);
	} else {
	    printf("Error (bad move): %s\n", line);
	    return 1;
	}

	const uint32_t nmoves = gen_legal_moves(&position, &moves[0]);
	fprintf(settings->log, "Found %d legal moves\n", nmoves);
	int zz = nmoves < 10 ? nmoves : 10;
	for (int ii = 0; ii < zz; ++ii) {
	    fprintf(settings->log, "\t%s\n", xboard_move_print(moves[ii]));
	}
	if (nmoves == 0) {
	    // TODO(plesslie): send correct end of game state
	    printf("resign\n");
	} else {
	    const uint32_t r = rand() % nmoves;
	    make_move(&position, moves[r], &sp);
	    const char *movestr = xboard_move_print(moves[r]);

	    fprintf(settings->log, "Trying to make move: %s\n", movestr);
	    printf("move %s\n", movestr);
	}
    }
    
    return 0;
}

static void sighandler(int signum) {
    // mask sigint signal until i get pondering
}

static const char * xboard_move_print(move mv) {
    // max move length is "e7e8q", most moves are "e7e8"
    static char buffer[7];
    const char *pcs = " nbrq";
    const uint32_t to = TO(mv);
    const uint32_t from = FROM(mv);
    const uint32_t flags = FLAGS(mv);

    sprintf(&buffer[0], "%s%s", sq_to_str[from], sq_to_str[to]);
    if (flags == FLG_PROMO) {
	buffer[5] = pcs[PROMOPC[PROMO_PC(mv)]];
	buffer[6] = 0;
    } else {
	buffer[5] = 0;
    }
    return &buffer[0];
}

//================================================================================
// Tests
//================================================================================

// --- Test Structures ---
struct expected_t {
    uint64_t nodes;
    uint64_t captures;    
    uint64_t enpassants;
    uint64_t castles;    
    uint64_t promotions;    
    uint64_t checks;    
    uint64_t checkmates;
};

// --- Global Test Data ---
uint64_t checks     = 0;
uint64_t captures   = 0;
uint64_t enpassants = 0;
uint64_t castles    = 0;
uint64_t promotions = 0;
uint64_t checkmates = 0;

// --- Static Test Function Prototypes ---
static void reset_counts();
static int perft_count_test();
static int perft_count_test_ex(const char *name, const char *fen, const struct expected_t *expected, int max_depth);
static int perft_bulk_test_ex(const char *name, const char *fen, const uint64_t *expected, int max_depth);
static uint64_t perft(int depth, struct position *const restrict pos, move pmove, int cap);
static int test_make_move_ex(const char *fen, const move *moves);
static int test_undo_move_ex(const char *fen, const move *moves);
static int test_move_creation();
static int position_cmp(const struct position *restrict l, const struct position *restrict r);

// --- Test Functions ---
static void reset_counts() {
    checks     = 0;
    captures   = 0;
    enpassants = 0;
    castles    = 0;
    promotions = 0;
    checkmates = 0;
}

void test_perft(int argc, char **argv) {
    if (perft_count_test() != 0) {
        fputs("FAILURE!!\n", stderr);
    } else {
        fputs("Success.\n", stdout);
    }    
}

void test_deep_perft(int argc, char **argv) {
    const int max_depth = argc == 2 ? 8 : atoi(argv[2]);
    int depth;
    uint64_t nodes;
    struct position pos;
    struct timespec start, end;
    uint64_t nanos;

    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return;
    }    

    for (depth = 0; depth < max_depth; ++depth) {
        reset_counts();
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        nodes = perft(depth, &pos, 0, EMPTY);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        nanos = (((end.tv_sec * 1000000000ull) + end.tv_nsec) -
                 ((start.tv_sec   * 1000000000ull) + start.tv_nsec));
        
        printf("Perft(%u): Nodes(%" PRIu64 ") Captures(%" PRIu64 ") "
               "E.p.(%" PRIu64 ") Castles(%" PRIu64 ") "
               "Promotions(%" PRIu64 ") Checks(%" PRIu64 ") "
               "Checkmates(%" PRIu64 "). Nanos = %" PRIu64 "\n",
               depth, nodes, captures, enpassants, castles, promotions,
               checks, checkmates, nanos);
    }
}

static int perft_count_test() {
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
        nodes = perft(depth, &pos, 0, EMPTY);
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
        nodes = perft(depth, &pos, 0, EMPTY);
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

static uint64_t perft(int depth, struct position *const restrict pos, move pmove, int cap) {
    uint32_t i;
    uint32_t nmoves;
    uint64_t nodes = 0;
    struct savepos sp;
    move moves[MAX_MOVES];
    
    if (depth == 0) {
#define COUNTS
#ifdef COUNTS
        if (pmove != 0) {
            if (in_check(pos, pos->wtm) != 0) {
                ++checks;
            }
            if (cap != EMPTY) {
                ++captures;
            }
            if (FLAGS(pmove) == FLG_CASTLE) {
                ++castles;
            } else if (FLAGS(pmove) == FLG_EP) {
                ++enpassants;
                ++captures; // e.p. don't set the sp->captured_pc flag
            } else if (FLAGS(pmove) == FLG_PROMO) {
                ++promotions;
            }
        }
#endif
        return 1;
    }

    //nmoves = generate_moves(pos, &moves[0]);
    nmoves = gen_legal_moves(pos, &moves[0]);
    for (i = 0; i < nmoves; ++i) {
        make_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
	nodes += perft(depth - 1, pos, moves[i], sp.captured_pc);
        undo_move(pos, moves[i], &sp);
        assert(validate_position(pos) == 0);
    }

    return nodes;
}

void test_make_move(int argc, char **argv) {
    if (test_move_creation() != 0) {
        return;
    }

    printf("Running make move tests...\n");
    const char *start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";    
    move start_pos_moves[] = {
        MOVE(A2, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(A2, A4, MV_FALSE, MV_FALSE, MV_FALSE),        
        MOVE(B2, B3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B2, B4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        0
    };
    if (test_make_move_ex(start_pos_fen, &start_pos_moves[0]) != 0) {
        printf("Failed test for moves from starting position!\n");
        return;
    }

    const char *kiwi_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    move kiwi_moves[] = {
        MOVE(E2, A6, MV_FALSE, MV_FALSE, MV_FALSE), // bishop captures bishop
        MOVE(G2, H3, MV_FALSE, MV_FALSE, MV_FALSE), // pawn captures pawn
        MOVE(D2, G5, MV_FALSE, MV_FALSE, MV_FALSE), // bishop move
        MOVE(E1, G1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle king side
        MOVE(E1, C1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle queen side
        MOVE(F3, H3, MV_FALSE, MV_FALSE, MV_FALSE), // queen captures pawn
        MOVE(H1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // rook move
        MOVE(E1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // king move (not castling)
        0
    };
    if (test_make_move_ex(kiwi_fen, &kiwi_moves[0]) != 0) {
        printf("Failed test for moves from kiwi position!\n");
        return;
    }

    const char *ep_fen = "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6";
    move ep_moves[] = {
        MOVE(E5, D6, MV_FALSE, MV_TRUE, MV_FALSE),
        0
    };
    if (test_make_move_ex(ep_fen, &ep_moves[0]) != 0) {
        printf("Failed test for moves from e.p. position!\n");
        return;
    }

    const char *promo_fen = "8/1Pp5/7r/8/KR1p1p1k/8/4P1P1/8 w - -";
    move promo_moves[] = {
        MOVE(B7, B8, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE), // knight promo
        MOVE(B7, B8, MV_PRM_BISHOP, MV_FALSE, MV_FALSE), // bishop promo
        MOVE(B7, B8, MV_PRM_ROOK  , MV_FALSE, MV_FALSE), // rook promo
        MOVE(B7, B8, MV_PRM_QUEEN , MV_FALSE, MV_FALSE), // queen promo
        0
    };
    if (test_make_move_ex(promo_fen, &promo_moves[0]) != 0) {
        printf("Failed test for moves from promo position!\n");
        return;
    }
    
    printf("Succeeded.\n");
}

void test_undo_move(int argc, char **argv) {
    printf("Running undo move tests...\n");
    const char *start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";    
    move start_pos_moves[] = {
        MOVE(A2, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(A2, A4, MV_FALSE, MV_FALSE, MV_FALSE),        
        MOVE(B2, B3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B2, B4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(C2, C4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(D2, D4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(E2, E4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(F2, F4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G2, G4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(H2, H4, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, A3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(B1, C3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, F3, MV_FALSE, MV_FALSE, MV_FALSE),
        MOVE(G1, H3, MV_FALSE, MV_FALSE, MV_FALSE),
        0
    };
    if (test_undo_move_ex(start_pos_fen, &start_pos_moves[0]) != 0) {
        printf("Failed test for moves from starting position!\n");
        return;
    }

    const char *kiwi_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    move kiwi_moves[] = {
        MOVE(E2, A6, MV_FALSE, MV_FALSE, MV_FALSE), // bishop captures bishop
        MOVE(G2, H3, MV_FALSE, MV_FALSE, MV_FALSE), // pawn captures pawn
        MOVE(D2, G5, MV_FALSE, MV_FALSE, MV_FALSE), // bishop move
        MOVE(E1, G1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle king side
        MOVE(E1, C1, MV_FALSE, MV_FALSE, MV_TRUE ), // castle queen side
        MOVE(F3, H3, MV_FALSE, MV_FALSE, MV_FALSE), // queen captures pawn
        MOVE(H1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // rook move
        MOVE(E1, F1, MV_FALSE, MV_FALSE, MV_FALSE), // king move (not castling)
        0
    };
    if (test_undo_move_ex(kiwi_fen, &kiwi_moves[0]) != 0) {
        printf("Failed test for moves from kiwi position!\n");
        return;
    }

    const char *ep_fen = "rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6";
    move ep_moves[] = {
        MOVE(E5, D6, MV_FALSE, MV_TRUE, MV_FALSE),
        0
    };
    if (test_undo_move_ex(ep_fen, &ep_moves[0]) != 0) {
        printf("Failed test for moves from e.p. position!\n");
        return;
    }

    const char *promo_fen = "8/1Pp5/7r/8/KR1p1p1k/8/4P1P1/8 w - -";
    move promo_moves[] = {
        MOVE(B7, B8, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE), // knight promo
        MOVE(B7, B8, MV_PRM_BISHOP, MV_FALSE, MV_FALSE), // bishop promo
        MOVE(B7, B8, MV_PRM_ROOK  , MV_FALSE, MV_FALSE), // rook promo
        MOVE(B7, B8, MV_PRM_QUEEN , MV_FALSE, MV_FALSE), // queen promo
        0
    };
    if (test_undo_move_ex(promo_fen, &promo_moves[0]) != 0) {
        printf("Failed test for moves from promo position!\n");
        return;
    }

    printf("Success.\n");
}

static int test_make_move_ex(const char *fen, const move *moves) {
    struct position pos;
    struct position tmp;
    struct savepos sp;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }
    memcpy(&tmp, &pos, sizeof(tmp));

    const move *m = moves;
    while (*m) {
        #ifdef EXTRA_INFO
        printf("Testing: "); move_print(*m);
        #endif
        
        make_move(&pos, *m, &sp);
        if (validate_position(&pos) != 0) {
            fputs("Failed to make move!\n", stderr);
            return 1;
        }
        // restore pos
        memcpy(&pos, &tmp, sizeof(tmp));
        ++m;
    }

    return 0;
}

static int test_undo_move_ex(const char *fen, const move *moves) {
    struct position pos;
    struct position tmp;
    struct savepos sp;

    if (read_fen(&pos, fen, 0) != 0) {
        fputs("Failed to read FEN for position!", stderr);
        return 1;
    }
    memcpy(&tmp, &pos, sizeof(tmp));

    const move *m = moves;
    while (*m) {
        #ifdef EXTRA_INFO
        printf("Testing: "); move_print(*m);
        #endif
        
        make_move(&pos, *m, &sp);
        if (validate_position(&pos) != 0) {
            fputs("Failed to make move!\n", stderr);
            return 1;
        }
        
        // restore pos
        undo_move(&pos, *m, &sp);
        if (validate_position(&pos) != 0) {
            fputs("validate_position failed after calling undo move!\n", stderr);
            return 1;
        }
        if (position_cmp(&pos, &tmp) != 0) {
            fputs("position_cmp failed after undo_move()\n", stderr);
            return 1;            
        }
        if (memcmp(&pos, &tmp, sizeof(tmp)) != 0) {
            fputs("memcmp failed after undo_move()\n", stderr);
            return 1;
        }
        ++m;
    }

    return 0;    
}

static int test_move_creation() {
    // verify that move creation macro works correctly
    
    int i;
    int ret;
    struct test_t {
        uint32_t from;
        uint32_t to;
        uint32_t prm;        
        uint32_t ep;
        uint32_t csl;
    } tests[] = {
        { E2, E4, MV_FALSE     , MV_FALSE, MV_FALSE }, // regular case
        { E6, D5, MV_FALSE     , MV_TRUE , MV_FALSE }, // ep case
        { E8, E7, MV_PRM_QUEEN , MV_FALSE, MV_FALSE }, // prm case - white queen
        { E8, E7, MV_PRM_ROOK  , MV_FALSE, MV_FALSE }, // prm case - white rook
        { E8, E7, MV_PRM_BISHOP, MV_FALSE, MV_FALSE }, // prm case - white bishop
        { E8, E7, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE }, // prm case - white knight
        { E1, E2, MV_PRM_KNIGHT, MV_FALSE, MV_FALSE }, // prm case - black knight
        { G1, E1, MV_FALSE     , MV_FALSE, MV_TRUE  },
        { A1, H8, MV_FALSE     , MV_FALSE, MV_FALSE },
        { A1, B1, MV_FALSE     , MV_FALSE, MV_FALSE }
    };

    printf("Testing move creation...\n");
    for (i = 0; i < (sizeof(tests)/sizeof(tests[0])); ++i) {
        const move mv = MOVE(tests[i].from,
                                     tests[i].to,
                                     tests[i].prm,
                                     tests[i].ep,
                                     tests[i].csl);
        const uint32_t to    = TO(mv);
        const uint32_t from  = FROM(mv);
        const uint32_t prm   = PROMO_PC(mv);
        const uint32_t flags = FLAGS(mv);

        if (to != tests[i].to) {
            printf("to(%u) != tests[i].to(%u)\n", to, tests[i].to);
            ret = 1;
        } else if (from != tests[i].from) {
            printf("from(%u) != tests[i].from(%u)\n", from, tests[i].from);
            ret = 1;
        } else if (tests[i].prm != MV_FALSE && flags != FLG_PROMO) {
            printf("prm != FALSE && flags(%u) != FLG_PROMO\n", flags);
            ret = 1;
        } else if (tests[i].prm != MV_FALSE && prm != tests[i].prm) {
            printf("prm(%u) != tests[i].prm(%u)\n", prm, tests[i].prm);
            ret = 1;
        } else if (tests[i].ep == MV_TRUE && flags != FLG_EP) {
            printf("ep != FALSE && flags(%u) != FLG_EP\n", flags);
            ret = 1;
        } else if (tests[i].csl == MV_TRUE && flags != FLG_CASTLE) {
            printf("csl != FALSE && flags(%u) != FLG_CASTLE\n", flags);
            ret = 1;
        } else {
            ret = 0;
        }

        if (ret != 0) {
            printf("Failed on test case: (frm=%s, to=%s, prm=%u, ep=%u, csl=%u) sm = 0x%04" PRIx32 "\n",
                   sq_to_str[tests[i].from], sq_to_str[tests[i].to], tests[i].prm,
                   tests[i].ep, tests[i].csl, mv);
            return ret;
        }
                
    }
    printf("Success.\n");
    return 0;
}

static int position_cmp(const struct position *restrict l, const struct position *restrict r) {
    int i;
    for (i = PC(WHITE,PAWN); i <= PC(BLACK,KING); ++i) {
        if (l->brd[i] != r->brd[i]) {
            fprintf(stderr, "l->brd[%c] != r->brd[%c] 0x%08" PRIX64 " != 0x%08" PRIX64 "\n",
                    vpcs[i], vpcs[i], l->brd[i], r->brd[i]);
            return 1;
        }
    }

    for (i = 0; i < 64; ++i) {
        if (l->sqtopc[i] != r->sqtopc[i]) {
            fprintf(stderr, "(l->sqtopc[%s]=%c) != (r->sqtopc[%s]=%c)\n",
                    sq_to_str[i], vpcs[l->sqtopc[i]],
                    sq_to_str[i], vpcs[r->sqtopc[i]]);
            return 2;
        }
    }

    if (l->nmoves != r->nmoves) {
        fprintf(stderr, "l->nmoves(%u) != r->nmoves(%u)\n", l->nmoves, r->nmoves);
        return 3;
    }
    if (l->wtm != r->wtm) {
        fprintf(stderr, "l->wtm(%s) != r->wtm(%s)\n", SIDESTR(l->wtm), SIDESTR(r->wtm));
        return 4;
    }
    if (l->halfmoves != r->halfmoves) {
        fprintf(stderr, "l->halfmoves(%u) != r->halfmoves(%u)\n", l->halfmoves, r->halfmoves);
        return 5;
    }
    if (l->castle != r->castle) {
        fprintf(stderr, "l->castle(%u) != r->castle(%u)\n", l->castle, r->castle);
        return 6;
    }
    if (l->enpassant != r->enpassant) {
        fprintf(stderr, "l->enpassant(%u) != r->enpassant(%u)\n", l->enpassant, r->enpassant);
        return 7;
    }

    return 0;
}

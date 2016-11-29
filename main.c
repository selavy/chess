#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include "move.h"
#include "position.h"
#include "movegen.h"
#include "perft.h"
#include "magic_tables.h"

struct timespec diff(struct timespec start, struct timespec end){
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

#define MAX_DEPTH 6
struct test_position {
    const char *fen;
    uint64_t nodes[MAX_DEPTH];
};

int check_perft() {
    int i;
    int ret;
    struct position pos;
    uint64_t nodes;
    uint64_t captures;
    uint64_t eps;
    uint64_t castles;
    uint64_t promos;
    uint64_t checks;
    uint64_t mates;
    struct test_position test_positions[] = {
	{
	    .fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	    .nodes={ 1, 20, 400, 8902, 197281, 4865609 }
	},
	{
	    .fen="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	    .nodes={ 1, 48, 2039, 97862, 4085603, 193690690 }
	},
	{
	    .fen="8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
	    .nodes={ 1, 14, 191, 2812, 43238, 674624 }
	},
	{
	    .fen="r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
	    .nodes={ 1, 6, 264, 9467, 422333, 15833292 }
	},
	{
	    .fen="r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
	    .nodes={ 1, 6, 264, 9467, 422333, 15833292 }
	},
	{
	    .fen="rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
	    .nodes={ 1, 44, 1486, 62379, 2103487, 89941194 }
	},
	{
	    .fen="r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
	    //.nodes={ 1, 46, 2079, 89890, 3894594, 164075551 }
	    .nodes={ 1, 46, 2079, 89890, 0, 0 }
	},
	{
	    .fen=0,
	    .nodes={ 0, 0, 0, 0, 0, 0 }
	}
    };
    struct test_position *cur = &test_positions[0];
    while (cur->fen) {
	ret = position_from_fen(&pos, cur->fen);
	if (ret != 0) {
	    fprintf(stderr, "Unable to read fen for position! Error(%d), FEN = '%s'\n",
		    ret, cur->fen);
	    return 1;
	}
	position_print(stdout, &pos);
	printf("\n");

	ret = validate_position(&pos);
	if (ret != 0) {
	    fprintf(stderr, "Position validation failed! Error(%d), FEN = %s\n",
		    ret, cur->fen);
	    return 2;
	}

	for (i = 0; i < MAX_DEPTH && cur->nodes[i]; ++i) {
	    printf("Checking depth: %d...\n", i);
	    struct timespec begin, end, dur;
	    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
	    ret = perft_test(&pos, i, &nodes, &captures, &eps, &castles, &promos, &checks, &mates);
	    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	    if (ret != 0) {
		fprintf(stderr, "perft_test failed on depth = %d! Error(%d)\n", i, ret);
		return 3;
	    }
	    if (nodes != cur->nodes[i]) {
		fprintf(stderr, "Perft test failed.  Expected=%" PRIu64 ", Actual=%" PRIu64 "\n",
			cur->nodes[i], nodes);
		return 4;
	    } else {
		dur = diff(begin, end);
		printf("Passed depth %d, nodes = %" PRIu64 ", took %ld seconds %ld millis\n",
		       i, nodes, dur.tv_sec, dur.tv_nsec / 1000000);
	    }
	}
	
	++cur;
    }

    return 0;
}

void time_test(int depth) {
    uint64_t nodes = 0;
    struct timespec begin;
    struct timespec end;
    struct timespec dur;    
    struct position pos;
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    position_from_fen(&pos, fen);

    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    nodes = perft_speed(&pos, depth);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    dur = diff(begin, end);
    printf("Passed depth %d, nodes = %" PRIu64 ", took %ld seconds %ld millis\n",
	   depth, nodes, dur.tv_sec, dur.tv_nsec / 1000000);
}

int main(int argc, char **argv) {
    #if 0
    // test make_move() and undo_move() on some basic positions
    int i;    
    int ret;
    int nmoves;    
    struct position pos;
    struct position tmp;    
    struct savepos sp;
    move moves[MAX_MOVES];
    const char *fen[] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
	"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
	"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
	"rnbqkbnr/p1ppp1pp/1p6/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 2",
	"rnbqkbnr/ppp1pppp/8/8/3pP3/P6P/1PPP1PP1/RNBQKBNR b KQkq e3 0 2",
	"rnbqkb1r/ppp1pppp/5n2/8/3pP3/P2B1N1P/1PPP1PP1/RNBQK2R w KQkq - 1 4",
	"rnbqkbnr/p1pppppp/8/8/1p6/3P4/PPPKPPPP/RNBQ1BNR w kq - 0 3",
	0
    };
    
    const char **cur = &fen[0];
    while (*cur) {
	ret = position_from_fen(&pos, *cur);
	if (ret != 0) {
	    fprintf(stderr, "Unable to read fen for position! Error(%d), FEN = %s\n",
		    ret, *cur);
	    exit(EXIT_FAILURE);
	}
	position_print(stdout, &pos);
	printf("\n");
	
	ret = validate_position(&pos);
	if (ret != 0) {
	    fprintf(stderr, "Position validation failed! Error(%d), FEN = %s\n",
		    ret, *cur);
	    exit(EXIT_FAILURE);
	}

	memset(&moves[0], 0, sizeof(moves[0]) * MAX_MOVES);
	memcpy(&tmp, &pos, sizeof(tmp));
	nmoves = generate_legal_moves(&pos, &moves[0]);
	printf("Legal moves:\n");
	for (i = 0; i < nmoves; ++i) {
	    move_print(moves[i]);
	    make_move(&pos, &sp, moves[i]);
	    assert(validate_position(&pos) == 0);
	    undo_move(&pos, &sp, moves[i]);
	    assert(validate_position(&pos) == 0);
	    assert(memcmp(&tmp, &pos, sizeof(tmp)) == 0);
	}
	printf("\n");
	++cur;	
    }
    #endif

    #if 0
    // verify perft values on some known positions
    printf("checking perft values...\n");
    if (check_perft() != 0) {
	printf("check perft failed!\n");
    } else {
	printf("passed.\n");
    }
    #endif

    #if 1
    // time starting position perft to given depth
    time_test(7);
    #endif

    #if 0
    // for each of the moves in the position, print number of moves on that branch to `depth'
    
    //int depth = 5; const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    //int depth = 4; const char *fen = "rnbqkbnr/pppppppp/8/8/8/2P5/PP1PPPPP/RNBQKBNR b KQkq - 0 1";
    //int depth = 3; const char *fen = "rnbqkbnr/pppp1ppp/8/4p3/8/2P5/PP1PPPPP/RNBQKBNR w KQkq e6 0 2";
    //int depth = 2; const char *fen = "rnbqkbnr/pppp1ppp/8/4p3/Q7/2P5/PP1PPPPP/RNB1KBNR b KQkq - 1 2";
    //int depth = 1; const char *fen = "rnb1kbnr/pppp1ppp/8/4p3/Q6q/2P5/PP1PPPPP/RNB1KBNR w KQkq - 2 3";
    //int depth = 4; const char *fen = "rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
    //int depth = 3; const char *fen = "rnbqkb1r/pppppppp/5n2/8/8/3P4/PPP1PPPP/RNBQKBNR w KQkq - 1 2";
    int depth = 2; const char *fen = "rnbqkb1r/pppppppp/5n2/8/8/3P4/PPPKPPPP/RNBQ1BNR b kq - 2 2";
    
    struct position pos;
    if (position_from_fen(&pos, fen) != 0) {
	perror("position_from_fen");
	exit(EXIT_FAILURE);
    }
    if (validate_position(&pos) != 0) {
	perror("position_from_fen");
	exit(EXIT_FAILURE);	
    }
    perft_text_tree(&pos, depth);
    #endif

#define CREATE_POSITION_FROM_FEN(pos, fen) do {				\
	if (position_from_fen(&(pos), (fen)) != 0) exit(EXIT_FAILURE);	\
	if (validate_position(&(pos)) != 0) exit(EXIT_FAILURE);		\
    } while (0)
    
    #if 0
    // print all legal moves generated for `fen'
    const char *fen = "rnb1kbnr/pppp1ppp/8/4p3/Q6q/2P5/PP1PPPPP/RNB1KBNR w KQkq - 2 3";
    struct position pos;
    move moves[MAX_MOVES];

    CREATE_POSITION_FROM_FEN(pos, fen);

    const int nmoves = generate_legal_moves(&pos, &moves[0]);
    for (int i = 0; i < nmoves; ++i) {
	move_print_short(moves[i]); printf("\n");
    }
    #endif

    
    #if 0
    // Test evasion move generation

    //const char *fen = "rnbqkb1r/pppppppp/8/8/4n3/3P4/PPPKPPPP/RNBQ1BNR w kq - 3 3";
    //const char *fen = "r1bqkbnr/1pp2pp1/n2p3p/pB2p1B1/3PP3/5N2/PPP2PPP/RN1QK2R b KQkq - 1 6";
    //const char *fen = "r1bqkbnr/1pp3pp/n2p4/pB2ppB1/3PP3/5N2/PPP2PPP/RN1QK2R b KQkq - 1 6";
    //const char *fen = "r1bqkbnr/1pp3pp/n2p1p2/pB2p1B1/3PP3/5N2/PPP2PPP/RN1QK2R b KQkq - 1 6";
    //const char *fen = "r1bq1bnr/1p4pp/n1pk1p2/pB2P1B1/4p3/5N2/PPPQ1PPP/RN1R3K b - - 0 11";
    //const char *fen = "2bq1bnr/rp4pp/n1pk1p1B/pB2P3/4p3/5N2/PPPQ1PPP/RN1R3K b - - 0 12";
    //const char *fen = "2bq1bnr/rp4pp/n1pk1p1B/pB2p3/1Q1Pp3/5N2/PPP2PPP/RN1R3K b - - 3 12";
    //const char *fen = "2bq2nr/rp2b1pp/n1pk1p1B/pB2p3/3PN3/8/PPPQ1PPP/RN1R3K b - - 0 13";
    //const char *fen = "2bq2nr/rp2b1pp/n1p1kp1B/pB1Pp3/4p3/7N/PPPQ1PPP/RN1R3K b - - 0 14";
    //const char *fen = "rnbqkbnr/1pp2ppp/p2p4/1B2p3/4P3/5N2/PPPP1PPP/RNBQ1RK1 b kq - 1 6";
    //const char *fen = "rnbq1bnr/1ppp1ppp/8/2k1p2P/pP2P3/5N2/P1PPBPP1/RNBQ1RK1 b - b3 0 10";
    const char *fen = "rnbq1bnr/1p1p1ppp/8/p1k1p2P/1Pp1P3/5NP1/P1PPBP2/RNBQ1RK1 b - b3 0 11";
    
    struct position pos;
    move moves[MAX_MOVES];
    CREATE_POSITION_FROM_FEN(pos, fen);

    position_print(stdout, &pos);

    const uint64_t checkers = generate_checkers(&pos, pos.wtm);
    printf("Checkers bb: %" PRIu64 "\n", checkers);

    const uint64_t attacked = generate_attacked(&pos, FLIP(pos.wtm));
    printf("Attacked bb: %" PRIu64 "\n", attacked);
    
    const move *cur = &moves[0];
    const move *end = generate_evasions(&pos, checkers, &moves[0]);
    while (cur != end) {
	move_print_short(*cur++); printf("\n");
    }
    #endif

    #if 0
    //const char *fen = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3";
    //const char *fen = "r1bqkbnr/ppp2ppp/2np4/1B2p3/4P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 4";
    const char *fen = "rn1qkbnr/ppp2ppp/2bp4/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 5 5";
    struct position pos;
    move moves[MAX_MOVES];
    CREATE_POSITION_FROM_FEN(pos, fen);
    position_print(stdout, &pos);
    const uint64_t pinned = pinned_pieces(&pos, pos.wtm, pos.wtm);
    printf("pinned: %" PRIu64 "\n", pinned);
    printf("to move: %s\n", pos.wtm == WHITE ? "WHITE" : "BLACK");

    int ret;
    const move *cur = &moves[0];
    const move *end = generate_non_evasions(&pos, &moves[0]);
    while (cur != end) {
	move_print_short(*cur); printf("\n");
	ret = is_legal_ex(&pos, pinned, *cur);
	printf("legal? %s\n", ret ? "Yes" : "No");
	++cur;
    }
    
    #endif
    
    return EXIT_SUCCESS;
}

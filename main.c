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

#define DEPTH 6
struct test_position {
    const char *fen;
    uint64_t nodes[DEPTH];
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
	{ .fen=0, .nodes={ 0, 0, 0, 0, 0, 0 } }
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

	for (i = 0; i < DEPTH; ++i) {
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

int main(int argc, char **argv) {
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

    printf("checking perft values...\n");
    if (check_perft() != 0) {
	printf("check perft failed!\n");
    } else {
	printf("passed.\n");
    }
    
    return EXIT_SUCCESS;
}

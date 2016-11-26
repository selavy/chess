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

int main(int argc, char **argv) {
    int ret;
    struct position pos;
    struct position tmp;    
    struct savepos sp;
    move moves[MAX_MOVES];
    int nmoves;
    int i;
    uint64_t nodes, captures, eps, castles, promos, checks, mates;
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

    printf("Begin perft...\n");
    cur = &fen[0];
    while (*cur) {
	ret = position_from_fen(&pos, *cur);
	if (ret != 0) {
	    fprintf(stderr, "Unable to read fen for position! Error(%d), FEN = %s\n",
		    ret, *cur);
	    exit(EXIT_FAILURE);
	}
	ret = validate_position(&pos);
	if (ret != 0) {
	    fprintf(stderr, "Position validation failed! Error(%d), FEN = %s\n",
		    ret, *cur);
	    exit(EXIT_FAILURE);
	}

	struct timespec begin, end, dur;
	for (i = 0; i < 8; ++i) {
	    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
	    perft_test(&pos, i, &nodes, &captures, &eps, &castles, &promos, &checks, &mates);
	    printf("%d: Nodes=%" PRIu64 ", "
		   "Captures=%" PRIu64 ", "
		   "E.p.=%" PRIu64 ", "
		   "Castles=%" PRIu64 ", "
		   "Promotions=%" PRIu64 ", "
		   "Checks=%" PRIu64 ", "
		   "Mates=%" PRIu64
		   "\n", 
		   i, nodes, captures, eps, castles, promos, checks, mates);
	    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	    dur = diff(begin, end);
	    printf("%ld seconds %ld millis\n", dur.tv_sec, dur.tv_nsec / 1000000);
	}
	break;
    }
    
    return EXIT_SUCCESS;
}

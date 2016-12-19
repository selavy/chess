#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include "magic_tables.h"
#include "move.h"
#include "position.h"
#include "movegen.h"
#include "perft.h"
#include "xboard.h"
#include "search.h"

#define CREATE_POSITION_FROM_FEN(pos, fen) do {				\
	if (position_from_fen(&(pos), (fen)) != 0) exit(EXIT_FAILURE);	\
	if (validate_position(&(pos)) != 0) exit(EXIT_FAILURE);		\
    } while (0)


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
    printf("Timing perft to depth %d from starting position...\n", depth);

    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    nodes = perft_speed(&pos, depth);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    dur = diff(begin, end);
    printf("Depth %d, nodes = %" PRIu64 ", took %ld seconds %ld millis\n",
	   depth, nodes, dur.tv_sec, dur.tv_nsec / 1000000);
}

void test_search() {
    struct position pos;
    const char *fen = "r1bqkbnr/pppppppp/8/8/1n1PP3/2N5/PPP2PPP/R1BQKBNR b KQkq - 2 3";
    position_from_fen(&pos, fen);
    printf("Searching from starting position...\n");
    move m = search(&pos);
    move_print(m);
    printf("Done.\n");
}

int main(int argc, char **argv) {
#if 1
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
    int depth = argc == 2 ? atoi(argv[1]) : 7;
    time_test(depth);
#endif

#if 0
    FILE *istream;
    char *line = 0;
    size_t len = 0;
    ssize_t read;
    int nchars;
    int rval;

    istream = fdopen(STDIN_FILENO, "rb");
    if (!istream) {
	perror("fdopen");
	exit(EXIT_FAILURE);
    }
    setbuf(stdout, 0);
    setbuf(istream, 0);
    
    while ((read = getline(&line, &len, istream)) > 0) {
	nchars = (int)read - 1;
	#define CHECKOPT(val) strncmp(line, val, strlen(val)) == 0
	if (CHECKOPT("check-perft")) {
	    printf("Checking perft...\n");
	    rval = check_perft();
	    printf("\n\nResult: %s\n", rval == 0 ? "Success!" : "Failure!");
	} else if (CHECKOPT("perft")) {
	    int depth = 7;
	    if (nchars > strlen("perft") + 1) {
		depth = atoi(line + strlen("perft") + 1);
	    }
	    time_test(depth);
	} else if (CHECKOPT("search")) {
	    test_search();
	} else if (CHECKOPT("xboard")) {
	    if (xboard_uci_main(istream) != 0) {
		perror("xboard_uci_main");
		exit(EXIT_FAILURE);
	    }
	    break;
	} else if (CHECKOPT("exit")) {
	    break;
	} else {
	    printf("Unrecognized command: '%.*s'\n", nchars, line);
	}
    }

    printf("Bye.\n");
    free(line);
    fclose(istream);
#endif
    
    return EXIT_SUCCESS;
}

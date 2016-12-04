#define _GNU_SOURCE
#include "xboard.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "move.h"
#include "position.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"

enum {
    XBOARD_SETUP,
    XBOARD_PLAYING,
};

#define DEBUGF(...) do { fprintf(g_settings->debug_output, __VA_ARGS__); } while(0)
#define WRITE(...) do { DEBUGF(__VA_ARGS__); printf(__VA_ARGS__); } while(0)

struct xboard_settings {
    int state;
    FILE *debug_output;
    struct position pos;
    struct savepos sp;
    move moves[MAX_MOVES];
};
// TEMP TEMP
struct xboard_settings *g_settings = 0;
static int xboard_settings_create(struct xboard_settings *settings) {
    settings->state = XBOARD_SETUP;
    settings->debug_output = fopen("/tmp/xboard_output.txt", "w");
    if (!settings->debug_output) {
	return 1;
    }
    setbuf(settings->debug_output, 0);
    memset(&settings->moves[0], 0, sizeof(settings->moves[0]));
    memset(&settings->pos, 0, sizeof(settings->pos));
    memset(&settings->sp, 0, sizeof(settings->sp));
    // TODO: move this to a common location
    const char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    if (position_from_fen(&settings->pos, starting_position) != 0) {
	return 1;
    }
    // TEMP TEMP
    g_settings = settings;
    return 0;
}
static int xboard_settings_destroy(struct xboard_settings *settings) {
    if (settings->debug_output) {
	fclose(settings->debug_output);
    }
    // TEMP TEMP    
    g_settings = 0;
    return 0;
}
static void sigh(int nsig) {
    DEBUGF("Received signal: %d\n", nsig);
}

static int xboard_handle_input(const char *line, int len, struct xboard_settings *settings) {
    DEBUGF("xboard_handle_input(%.*s)\n", len, line);

    signal(SIGINT, &sigh);
    
    #define STRNCMP(x, y) strncmp(x, y, strlen(y)) == 0
    #define STRCMP(x, y) strcmp(x, y) == 0
    if (settings->state == XBOARD_SETUP) {
	if (STRNCMP(line, "protover")) {
	    if (len < 10 || line[9] != '2') {
		//fprintf(stderr, "Unrecognized protocol version!\n");
		DEBUGF("Unrecognized protocol version!");
		return 1;
	    }
	    /* printf("feature myname=\"experiment\"\n"); */
	    /* printf("feature reuse=0\n"); */
	    /* printf("feature analyze=0\n"); */
	    /* printf("feature time=0\n"); */
	    /* printf("feature done=1\n"); */

	    WRITE("feature myname=\"experiment\"\n");
	    WRITE("feature reuse=0\n");
	    WRITE("feature analyze=0\n");
	    WRITE("feature time=0\n");
	    WRITE("feature done=1\n");
	} else if (STRCMP(line, "new")) {
	    // nop?
	} else if (STRCMP(line, "random")) {
	    // nop?
	} else if (STRNCMP(line, "level")) {
	    // TODO: parse time control
	} else if (STRCMP(line, "post")) {
            // TODO(plesslie):
            // turn on thinking/pondering output
            // thinking output should be in the format "ply score time nodes pv"
            // E.g. "9 156 1084 48000 Nf3 Nc6 Nc3 Nf6" ==>
            // "9 ply, score=1.56, time = 10.84 seconds, nodes=48000, PV = "Nf3 Nc6 Nc3 Nf6""
	} else if (STRNCMP(line, "accepted")) {
	    // nop?
	} else if (STRCMP(line, "hard")) {
	    settings->state = XBOARD_PLAYING;
	} else if (STRCMP(line, "white")) {
	    // TODO: setup side
	} else if (STRCMP(line, "black")) {
	    // TODO: setup side
	} else if (STRNCMP(line, "time")) {
	    // nop?
	} else if (STRNCMP(line, "otim")) {
	    // nop?
	} else if (STRCMP(line, "force")) {
	    // nop?
	    // stop thinking about current position
	} else {
	    //printf("Error (unknown command): %.*s\n", len, line);
	    WRITE("Error (unknown command): %.*s\n", len, line);
	    return 1;
	}
    } else if (settings->state == XBOARD_PLAYING) {
	if (STRCMP(line, "go")) {
	    // nop?
	} else if (STRCMP(line, "white")) {
	    // nop?
	    return 0;
	} else if (len == 4 || len == 5) {
	    const int from = (line[1]-'1')*8 + (line[0]-'a');
	    const int to   = (line[3]-'1')*8 + (line[2]-'a');
	    DEBUGF("Parsed move as %u -> %u, %s -> %s\n",
		   to, from, sq_to_str[from], sq_to_str[to]);
	    move m;
	    if (len == 4) {
		m = MOVE(from, to);
	    } else {
		int prm;
		switch (line[4]) {
		case 'q': prm = QUEEN; break;
		case 'r': prm = ROOK; break;
		case 'b': prm = BISHOP; break;
		case 'n': prm = KNIGHT; break;
		default:
		    //printf("Invalid move: %.*s\n", len, line);
		    WRITE("Invalid move: %.*s\n", len, line);
		}
		m = PROMOTION(from, to, prm);
	    }

	    make_move(&settings->pos, &settings->sp, m);
	} else {
	    //printf("Error (bad move): %.*s\n", len, line);
	    WRITE("Error (bad move): %.*s\n", len, line);
	    return 1;
	}
	
	/* const int nmoves = generate_legal_moves(&settings->pos, &settings->moves[0]); */
	/* if (nmoves == 0) { */
	/*     //printf("resign\n"); */
	/*     WRITE("resign\n"); */
	/*     return 1; // REVISIT: exit after resigning? */
	/* } else { */
	/*     int r = rand() % nmoves; */
	/*     make_move(&settings->pos, &settings->sp, settings->moves[r]); */
	/*     const char *movestr = xboard_move_print(settings->moves[r]); */
	/*     //printf("move %s\n", movestr); */
	/*     WRITE("move %s\n", movestr); */
	/* } */

	// TODO: resign logic? maybe just never resign...
	// REVISIT(plesslie): xboard isn't detecting mate.  need to figure out what to send there
	move mv = search(&settings->pos);
	make_move(&settings->pos, &settings->sp, mv);
	const char *movestr = xboard_move_print(mv);
	WRITE("move %s\n", movestr);
	
    } else {
	//printf("Error (invalid state): %d", settings->state);
	WRITE("Error (invalid state): %d", settings->state);
	return 1;
    }

    return 0;
}

/*extern*/ int xboard_uci_main(FILE *istream) {
    struct xboard_settings settings;
    char *line = 0;
    size_t len = 0;
    ssize_t read;

    if (xboard_settings_create(&settings) != 0) {
	return 1;
    }

    while ((read = getline(&line, &len, istream)) > 0) {
	line[read - 1] = 0;
	if (xboard_handle_input(line, read - 1, &settings) != 0) {
	    break;
	}
    }

    if (xboard_settings_destroy(&settings) != 0) {
	return 1;
    }
    return 0;
}

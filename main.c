#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "types.h"
#include "move.h"
#include "plchess.h"

void print_usage();
void do_perft();

struct command_t {
    const char *name;
    void (*func)(int argc, char **argv);
    const char *desc;
};
struct command_t commands[] = {
    { "--perft", &test_perft, "Run perft unit tests" },
    { "--help", &print_usage, "Print usage" },
    { "--make-move", &test_make_move, "Run make move tests" },
    { "--undo-move", &test_undo_move, "Run undo move tests" },
    { "--deep-perft", &test_deep_perft, "Run perft to a deep level -- for timing test" },
    { 0, 0, 0 }
};

void print_usage(int argc, char **argv) {
    struct command_t *cmd = &commands[0];
    printf("Usage: %s <MODE>\n", argv[0]);
    while (cmd->name) {
        printf("\t%s -- %s\n", cmd->name, cmd->desc);
        ++cmd;
    }
    printf("\n");
}

void sighandler(int signum) {
    // mask sigint signal until i get pondering
}

enum xboard_state { IDLE, SETUP, PLAYING };
struct xboard_settings {
    int state;
    FILE *log;
};
int xboard_settings_init(struct xboard_settings *settings, const char *log) {
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
int xboard_settings_finalize(struct xboard_settings *settings) {
    if (settings->log) {
	fclose(settings->log);
    }
    return 0;
}

#define XSTRNCMP(X,Y) strncmp(X, Y, strlen(Y))
int handle_xboard_input(const char * const line, size_t bytes, struct xboard_settings *settings) {
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

int main(int argc, char **argv) {
    if (argc < 2) {
        xboard_main();
    } else {
#ifdef NDEBUG
        printf("Built in `release' mode\n");
#else
        printf("Built in `debug' mode\n");
#endif
        void (*func)() = print_usage;
        struct command_t *cmd = &commands[0];
        while (cmd->name) {
            if (strcmp(argv[1], cmd->name) == 0) {
                func = cmd->func;
                break;
            }
            ++cmd;
        }
        func(argc, argv);
    }

    return 0;
}


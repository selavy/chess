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
#include "movegen.h"
#include "read_fen.h"
#include "perft.h"

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
    { "--deep-perft", &deep_perft, "Run perft to a deep level -- for timing test" },
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

void sighandler(int signum);
void xboard_main();
int handle_xboard_input(const char * const line, size_t bytes);

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

void sighandler(int signum) {
    // mask sigint signal until i get pondering
}

void xboard_main() {
    FILE *istream;
    char *line = 0;
    size_t len = 0;
    ssize_t read = 0;

    //printf("entering xboard mode...\n");

    // xboard sends SIGINT when the opponent moves - can use to wake
    // up from pondering once that is implemented
    signal(SIGINT, &sighandler);

    istream = fdopen(STDIN_FILENO, "rb");

    // turn off i/o buffering
    setbuf(stdout , NULL);
    setbuf(istream, NULL);

    while ((read = getline(&line, &len, istream)) > 0) {
        line[read-1] = 0;
        if (handle_xboard_input(line, read-1) != 0) {
            break;
        }
    }

    printf("Bye.\n");
    free(line);
    fclose(istream);
}

#define XSTRNCMP(X,Y) strncmp(X, Y, strlen(Y))
int handle_xboard_input(const char * const line, size_t bytes) {

    if (strcmp(line, "xboard") == 0) {
        // no-op
    } else if (XSTRNCMP(line, "protover") == 0) {
        if (line[9] != '2') {
            fprintf(stderr, "Unrecognized protocol version:\n");
            return 1;
        }
        printf("feature myname=\"experiment\"\n");
        printf("feature reuse=0\n");
        printf("feature analyze=0\n");
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
        // no-op
    } else if (strcmp(line, "easy") == 0) {
        // no-op
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
    } else if (strcmp(line, "go") == 0) {
        // TODO(plesslie):
        // begin game, if white, make first move
    } else {
        printf("Error (unknown command: %.*s\n", (int)bytes, line);
        return 1;
    }
    
    return 0;
}

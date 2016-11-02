#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include "plchess.h"

static void print_usage();

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

static void print_usage(int argc, char **argv) {
    struct command_t *cmd = &commands[0];
    printf("Usage: %s <MODE>\n", argv[0]);
    while (cmd->name) {
        printf("\t%s -- %s\n", cmd->name, cmd->desc);
        ++cmd;
    }
    printf("\n");
}

#if 0
static void sighandler(int signum) {
    printf("Received signal: %d\n", signum);
}

int main(int argc, char **argv) {
    FILE *istream;
    char *line = 0;
    size_t len = 0;
    ssize_t read;
    
    // xboard sends SIGINT when the opponent moves - can use to wake
    // up from pondering once that is implemented
    signal(SIGINT, &sighandler);

    istream = fdopen(STDIN_FILENO, "rb");
    if (!istream) {
	exit(EXIT_FAILURE);
    }
    // turn off i/o buffering
    setbuf(stdout , NULL);
    setbuf(istream, NULL);

    while ((read = getline(&line, &len, istream)) > 0) {
	const size_t perftsz = strlen("perft");
	if (strncmp(line, "perft", perftsz) == 0) {
	    int depth = 5;
	    if (read > (perftsz + 1)) {
		depth = atoi(line + perftsz);
	    }
	    printf("Running perft to depth %d!\n", depth);
	    break;
	} else {
	    printf("Unknown instruction: '%.*s'\n", (int)read-1, line);
	    break;
	}
    }

    printf("Bye.\n");
    free(line);
    fclose(istream);
    
    return 0;
}
#endif

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

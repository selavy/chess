#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
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

int main(int argc, char **argv) {
#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif

    if (argc < 2) {
        print_usage(argc, argv);
    } else {
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

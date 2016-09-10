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

const char *g_prog = 0;

struct command_t {
    const char *name;
    void (*func)();
    const char *desc;
};
struct command_t commands[] = {
    { "--perft", &do_perft, "Run perft unit tests" },
    { "--help", &print_usage, "Print usage" },
    { "--make-move-test", &make_move_test, "Run make move tests" },
    { 0, 0, 0 }
};

void print_usage() {
    struct command_t *cmd = &commands[0];

    printf("Usage: %s <MODE>\n", g_prog);
    while (cmd->name) {
        printf("\t%s -- %s\n", cmd->name, cmd->desc);
        ++cmd;
    }
    printf("\n");
}

void do_perft() {
    if (perft_count_test() != 0) {
        fputs("FAILURE!!\n", stderr);
    } else {
        fputs("Success.\n", stdout);
    }
}

int main(int argc, char **argv) {
    g_prog = argv[0];
#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif

    if (argc < 2) {
        print_usage();
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
        func();
    }

    return 0;
}

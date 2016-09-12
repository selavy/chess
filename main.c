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
    { "--perft", &test_perft, "Run perft unit tests" },
    { "--help", &print_usage, "Print usage" },
    { "--make-move", &test_make_move, "Run make move tests" },
    { "--undo-move", &test_undo_move, "Run undo move tests" },
    { "--deep-perft", &deep_perft, "Run perft to a deep level -- for timing test" },
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

int PopCnt(uint64_t x) {
    int c;
    for (c = 0; x; c++) {
        x &= x - 1;
    }
    return c;
}

int LSB(uint64_t arg1) {
    return __builtin_ffsll(arg1);
}

int main(int argc, char **argv) {
    g_prog = argv[0];
#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif

    /* uint32_t from; */
    /* uint64_t bbrd = MASK(60) | MASK(10); */
    /* while (bbrd) { */
    /*     from = __builtin_ctzll(bbrd); */
    /*     printf("%u\n", from); */
    /*     bbrd &= (bbrd - 1); */
    /* } */
    /* return 0; */

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

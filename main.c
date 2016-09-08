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

int main(int argc, char **argv) {
#ifdef NDEBUG
    printf("Built in `release' mode\n");
#else
    printf("Built in `debug' mode\n");
#endif
    
    if (perft_count_test() != 0) {
        fputs("FAILURE!!\n", stderr);
    } else {
        fputs("Success.\n", stdout);
    }

    return 0;
}

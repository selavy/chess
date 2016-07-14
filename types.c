#include "types.h"
#include <string.h>

int board_init(struct board * restrict brd) {
    memset(&brd->white[0], 0, sizeof(brd->white[0]) * NUM_PIECES * 2);
    return 0;
}

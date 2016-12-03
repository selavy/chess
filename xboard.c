#include "xboard.h"
#include "move.h"
#include "position.h"
#include "movegen.h"

enum {
    XBOARD_SETUP,
    XBOARD_PLAYING,
};

struct xboard_settings {
    int state;
    struct position pos;
    struct savepos sp;    
    move moves[MAX_MOVES];
};

static int xboard_settings_create(struct xboard_settings *settings) {
    return 0;
}

static int xboard_settings_destroy(struct xboard_settings *settings) {
    return 0;
}

/*extern*/ int xboard_uci_main() {
    struct xboard_settings settings;
    if (xboard_settings_create(&settings) != 0) {
	return 1;
    }

    

    if (xboard_settings_destroy(&settings) != 0) {
	return 1;
    }
    return 0;
}

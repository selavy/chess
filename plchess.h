#ifndef PLCHESS__H_
#define PLCHESS__H_

// --- Interface Functions ---

extern void xboard_main();

// --- Test Functions ---

extern void test_perft(int argc, char **argv);

extern void test_deep_perft(int argc, char **argv);

extern void test_make_move(int argc, char **argv);

extern void test_undo_move(int argc, char **argv);

#endif // PLCHESS_H_

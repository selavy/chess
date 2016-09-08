#ifndef READ_FEN__H_
#define READ_FEN__H_

struct position;

extern int read_fen(struct position * restrict pos, const char * const fen, int print);

#endif // READ_FEN__H_

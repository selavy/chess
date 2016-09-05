#!/bin/sh

make && \
    ./chess > ccout && \
    sort ccout > ccout.sorted && \
    ./perft.py > pyout && \
    sort pyout > pyout.sorted && \
    diff ccout.sorted pyout.sorted > differences && \
    ./print_fake_fen_position.py differences
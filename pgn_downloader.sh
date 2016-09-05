#!/bin/sh

echo "Downloading game id $1"

wget "http://www.chessgames.com/perl/nph-chesspgn?text=1&gid=$1" -O $1.pgn

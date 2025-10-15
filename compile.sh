#!/usr/bin/env bash

# Compile with ncurses
echo "Compiling... with arguments: $@";

# call without arguments => run toni
if [ $# -eq 0 ]; then set -- 't'; fi
echo $@

# Build & run
if [ $(expr index $@ c) -gt 0 ]; then
    gcc -g racing_chatgpt.c -lncurses -lm -o chatgpt;
    ./chatgpt
elif [ $(expr index $@ g) -gt 0 ]; then
    gcc -g racing_gemini.c  -lncurses -lm -o gemini;
    ./gemini
elif [ $(expr index $@ k) -gt 0 ]; then
    gcc -g racing_grock.c   -lncurses -o grock;
    ./grock
else
    gcc -g racing_t.c -lncurses -o toni;
    echo "Running toni ..."; 
    ./toni
fi
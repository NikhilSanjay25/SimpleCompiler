#!/bin/bash
./compiler < "$1"
llc out.ll -filetype=obj -relocation-model=pic -o -out.o
clang -fPIE out.o -o myprog
./myprog


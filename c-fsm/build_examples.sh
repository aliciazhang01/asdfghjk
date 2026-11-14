#!/usr/bin/env bash

mkdir -p build
for cfile in examples/*.c
do
    echo "Building $cfile"
    gcc -Wall -I. -o "build/$(basename "$cfile" .c)" "$cfile"
done
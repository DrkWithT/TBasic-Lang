#!/bin/bash

mode="$1";

if [ $mode = "cl" ]
then
    rm -rf ./build/ || printf "Cleaned build files";
elif [ $mode = "bd" ]
then
    cmake -S . -B build -DCMAKE_BUILD_TYPE:STRING=Debug && cmake --build build;
elif [ $mode = "br" ]
then
    cmake -S . -B build -DCMAKE_BUILD_TYPE:STRING=Release && cmake --build build;
    strip ./build/tbasic -o ./tbasic;
elif [ $mode = "bp" ]
then
    cmake -S . -B build -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo && cmake --build build;
elif [ $mode = "sd" ]
then
    cmake -S . -B build -DCMAKE_BUILD_TYPE:STRING=Debug -DCREATE_TBASIC_SHARED:BOOL=1 && cmake --build build;
elif [ $mode = "sr" ]
then
    cmake -S . -B build -DCMAKE_BUILD_TYPE:STRING=Release -DCREATE_TBASIC_SHARED:BOOL=1 && cmake --build build;
else
    printf "Usage:\n\tcl: clean build files\n\tbd: build debug\n\tbr: build release\n\tbp: build profile\n\tsd: build shared lib & demo (debug)\n\tsr: build shared lib & demo (release)\n";
fi

#!/bin/bash

argc=$#;

if [ $argc -ne 3 ]
then
    printf "Expected 3 arguments:\nUSAGE: microbench.sh (warmups) (runs) (bench name)";
    exit 1;
fi

warms=$1;
runs=$2;
bench=$3;

hyperfine -w $warms -r $runs --command-name=py3bench "python3 ./benchmarks/py3/$bench.py" --command-name=tbbench "./tbasic -r ./benchmarks/tb/$bench.tbasic" || printf "\033[1;31mFAILED TO MICRO-BENCH: invalid invocation of interpreter(s) / invalid file.";

#!/bin/bash

module use /projects/comp422/modulefiles/
module load cilkplus
module load hpctoolkit

hpcstruct othello
hpcrun -e REALTIME@1000 -t ./othello < input
hpcprof -S othello.hpcstruct hpctoolkit-othello-measurements/
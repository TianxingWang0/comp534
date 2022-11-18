#!/bin/bash

make clean
module use /projects/comp422/modulefiles/
module load cilkplus
make
./submit.sbatch
#make runp W=$1
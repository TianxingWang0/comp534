#!/bin/bash

make clean
module use /projects/comp422/modulefiles/
module load cilkplus
make
make screen
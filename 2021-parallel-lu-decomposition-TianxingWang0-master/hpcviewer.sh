#!/bin/bash

module use /projects/comp422/modulefiles
module load assignment2
module load hpctoolkit-2021.10

hpcviewer hpctoolkit-lu-omp-database*

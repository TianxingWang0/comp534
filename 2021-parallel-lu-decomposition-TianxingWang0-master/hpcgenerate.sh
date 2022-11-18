#!/bin/bash
rm -rf hpctoolkit-lu-omp-*

module use /projects/comp422/modulefiles
module load assignment2
module load hpctoolkit-2021.10

hpcstruct lu-omp
hpcrun -e ivb_ep::MEM_LOAD_UOPS_LLC_MISS_RETIRED:REMOTE_DRAM -e ivb_ep::MEM_LOAD_UOPS_LLC_MISS_RETIRED:LOCAL_DRAM ./lu-omp 7000 16
hpcprof -S lu-omp.hpcstruct hpctoolkit-lu-omp-measurements*/
# hpcrun -L ./lu-omp 7000 8 > output
#!/bin/bash

# rm -rf hpctoolkit-mmultiply-measurements*/ hpctoolkit-mmultiply-database*/
# rm mmultiply.hpcstruct

module purge
module use /projects/comp422/modulefiles
module load assignment3
module load hpctoolkit

# hpcstruct mmultiply
# srun -n 32 hpcrun -e REALTIME@1000 -t ./mmultiply 1600 2 0
# hpcprof -S mmultiply.hpcstruct hpctoolkit-mmultiply-measurements*/
hpcviewer hpctoolkit-mmultiply-database*
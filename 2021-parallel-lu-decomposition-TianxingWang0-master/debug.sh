make clean

module use /projects/comp422/modulefiles
module load assignment2
module load hpctoolkit

make
gdb lu-omp-serial
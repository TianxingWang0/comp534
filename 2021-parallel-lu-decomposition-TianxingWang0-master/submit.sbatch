#!/bin/bash
#SBATCH --export=ALL
#SBATCH --nodes=1 
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1 
#SBATCH --cpus-per-task=32
#SBATCH --exclusive
#SBATCH --time=00:05:00 
#SBATCH --partition=scavenge

export KMP_AFFINITY=verbose # just to print the bindings
for ((i = 1; i <= 32; i++)); do 
  OMP_PROC_BIND=spread time ./lu-omp 8000 $i
done

module use /projects/comp422/modulefiles
module load assignment4

srun --pty ${COMP422_RESERVATION} ${COMP422_PARTITION} --export=ALL --nodes=1 ${COMP422_GPU} --ntasks-per-node=1  --cpus-per-task=2 --mem-per-cpu=512 --time=00:30:00 bash


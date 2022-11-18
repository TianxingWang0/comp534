srun --pty --partition=interactive --export=ALL --nodes=2 --ntasks-per-node=16 --cpus-per-task=1 --ntasks-per-socket=16 --exclusive --mem-per-cpu=512 --time=00:30:00 bash

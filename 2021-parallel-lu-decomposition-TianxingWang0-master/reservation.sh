srun --pty --reservation=comp422 --partition=interactive --export=ALL --nodes=1 --ntasks-per-node=1 --cpus-per-task=24 --mem-per-cpu=512 --cpu-bind=none --time=00:30:00 bash

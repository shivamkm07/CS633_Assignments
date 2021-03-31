#! /bin/bash
rm -f output data
touch output
make clean
make
for i in {1..10}
do
  for P in 4 16
  do
    for ppn in 1 8
    do
      let ng=$P/6+1
      python group_nodes.py 6 $ng $ppn
      for d in 16 256 2048
      do
        mpirun -np $P*$ppn -f group_hostfile ./exec $d >> output
      done
    done
  done
done
python3 plot.py

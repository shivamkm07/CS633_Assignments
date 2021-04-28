#! /bin/bash
rm -f collected_data
touch output
make clean
make
for i in 1
do
  for P in 1 2
  do
    for ppn in 1 2 4
    do
      let num_nodes_pergroup=$P
      python group_nodes.py 1 $num_nodes_pergroup $ppn
      mpirun -np $P*$ppn -f group_hostfile ./exec tdata.csv
      tail -n 1 output.txt >> collected_data
    done
  done
done
#python3 plot.py

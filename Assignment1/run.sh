rm output data
touch output
make clean
make
for i in 1 2 3 4 5
do
  for P in 16 36 49 64 
  do
    ~/UGP/allocator/src/allocator.out $P 8 > /dev/null
    for N in 256 1024 4096 16384 65536 262144 1048576
    do
      mpirun -np $P -f hosts ./exec $N 50 >> output
    done
  done
done
python3 plot.py

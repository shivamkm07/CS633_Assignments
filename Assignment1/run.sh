SECONDS=0
rm output
touch output
mpicc halo.c -o halo -lm
for i in 1 2 3 4 5
do
  for P in 16 36 49 64 
  #for P in 16 
  do
    ~/UGP/allocator/src/allocator.out $P 8 > /dev/null
    cat hosts
    for N in 256 1024 4096 16384 65536 262144 1048576
    do
      echo "P "$P "N "$N
      mpirun -np $P -f hosts ./halo $N 50 >> output
    done
  done
done
echo "Total time in seconds "$SECONDS

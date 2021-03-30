rm -f output1 data1
touch output1
#chmod u+x create_hostfile
#make clean
#make
for i in 1
do
  for P in 4 16
  do
    for ppn in 1 8
    do
      let ng=$P/6+1
      python group1_nodes.py 6 $ng $ppn
      for d in 16 256 2048
      do
        echo "i "$i " P "$P" ppn "$ppn" d "$d
        mpirun -np $P*$ppn -f group1_hostfile ./exec1 $d >> output1
      done
    done
  done
done
#
##    ./create_hostfile $P 8 > /dev/null
#
#    for N in 256 1024 4096 16384 65536 262144 1048576
#    do
#      mpirun -np $P -f hosts ./exec $N 50 >> output
#    done
#  done
#done
#python3 plot.py

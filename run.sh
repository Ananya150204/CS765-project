make clean
make
./simulator --num_peers 30 --slow_percent 80 --low_cpu_percent 20 --transaction_mean_time 5000000 --block_mean_time 500000000 --end_time 12000000000 > out.txt
python3 src/visualize.py




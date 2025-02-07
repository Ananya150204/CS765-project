make clean
make
./simulator --num_peers 20 --slow_percent 80 --low_cpu_percent 20 --transaction_mean_time 5000000 --block_mean_time 28000000 --end_time 1200000000 > out.txt
python3 src/visualize.py




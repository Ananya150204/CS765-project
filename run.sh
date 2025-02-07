make clean
make
./simulator --peers 20 --slow_percent 80 --low_cpu_percent 20 --txn_interarrival 5000000 --block_interarrival_time 28000000 --end_time 1200000000 > out.txt
python3 src/visualize.py




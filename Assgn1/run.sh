make clean
make
./simulator --peers 50 --slow_percent 80 --low_cpu_percent 20 --txn_interarrival 500000 --block_interarrival_time 10000000 --end_time 120000000
python3 src/visualize.py




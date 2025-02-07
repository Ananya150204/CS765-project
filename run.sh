make clean
make
./simulator --peers 50 --slow_percent 100 --low_cpu_percent 100 --txn_interarrival 10000000 --block_interarrival_time 600000000 --end_time 12000000000 > out.txt
python3 src/visualize.py




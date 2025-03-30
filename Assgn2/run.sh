make clean
make
./simulator --peers 100 --slow_percent 20 -mnp 15 --txn_interarrival 500000 -Tt 2000 --block_interarrival_time 10000000 --end_time 1000000000 
python3 src/visualize.py




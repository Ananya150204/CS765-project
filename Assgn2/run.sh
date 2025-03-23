make clean
make
./simulator --peers 50 --slow_percent 30 -mnp 30 --txn_interarrival 500000 -Tt 2000000 --block_interarrival_time 10000000 --end_time 100000000 -nec
python3 src/visualize.py




make clean
make
./simulator --peers 50 --slow_percent 30 -mnp 40 --txn_interarrival 500000 -Tt 2000000 --block_interarrival_time 10000000 --end_time 1000000000 -nec
python3 src/visualize.py




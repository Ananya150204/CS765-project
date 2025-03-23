make clean
make
./simulator --peers 100 --slow_percent 20 -mnp 5 --txn_interarrival 500000 -Tt 200 --block_interarrival_time 10000000 --end_time 100000000 -nec
python3 src/visualize.py




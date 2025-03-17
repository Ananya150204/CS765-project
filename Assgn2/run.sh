make clean
make
./simulator --peers 50 --slow_percent 30 -mnp 60 --txn_interarrival 500000 -Tt 20 --block_interarrival_time 10000000 --end_time 120000000
python3 src/visualize.py




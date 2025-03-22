make clean
make
./simulator --peers 50 --slow_percent 30 -mnp 60 --txn_interarrival 500000 -Tt 20000000 --block_interarrival_time 10000000 --end_time 580000000 
python3 src/visualize.py




# Simulation of a P2P Cryptocurrency Network

## Instructor: Prof. Vinay Ribeiro, CS 765, IIT Bombay 

### Requirements
- Simulation
    - Make 
    - C++23 `Tested on GNU C++ (Ubuntu 22.04)`
- Visualization and Statistics
    - Python 3 `Recommended Python >= 3.6`
    - graphviz `sudo apt install graphviz`

### Compilation
To compile the simulator binary, simply run make. See Makefile for more information.
```
make
```

### Running
The simulator binary is named `simulator` and uses the following parameters.
```
Usage: ./simulator [options] 

Arguments:
-h --help                       shows help message and exits
-v --version                    prints version information and exits
-n --peers                      Number of peers in the network [default: 20]
-z0 --slow_percent              Percentage of slow nodes [default: 80]
-Tt --timeout                   Timeout value for get requests [default: 20]
-mnp --malicious_node_percent   Malicious node percentage [default: 20]
-Ttx --txn_interarrival         Mean transaction generation time (microseconds) [default: 5e+06]
-I --block_interarrival_time    Mean block generation time (microseconds) [default: 6e+08]
-e --end_time                   Simulation end time (microseconds) [default: 1.2e+10]
-nec --no-eclipse-attack        Disable eclipse attack [default: false]

```
Kindly note all the parameters have default values, so the executable will NOT report an error in case of absence of a parameter.

### Cleanup
To remove all the intermediate and output files and the binary, you can run
```
make clean
```

### Visualization
The simulator outputs the following files:
- `outputs/block_arrivals/*.csv` each txt file contains the arrival timestamps of each block at each peer, sorted by the block ID.
- `outputs/blockchains/*.txt` contains the edges of the blockchain formed at the termination point of the simulation (these blockchains are likely to be different at each peer, depending on the parameters set).
- `outputs/peer_stats.csv` each txt file contains the peer attributes (as per the final blockchain at that peer) relevant for post-simulation analysis.

`src/visualize.py` draws the blockchain tree by reading in the edgelist and saves it into the file `outputs/blockchain_images/*.png`. It also draws the peer_network and saves it as `outputs/peer_network_img.png`. It also draws the overlay network and saves it as `outputs/overlay_network_img.png`.


You can simply execute the python script to automatically perform the aforementioned task.
```
python3 src/visualize.py
```

### Internals
- The simulator will terminate as soon as the `time_limit` is reached.

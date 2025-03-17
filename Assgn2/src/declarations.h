#ifndef dec_H
#define dec_H
#include<bits/stdc++.h>
#include "argparse.h"
using namespace std;
typedef long double ld;
#define TXN_SIZE 8192    // in bits
#define MAX_BLK_SIZE 8388608    // in bits
#define MAX_TXNS 1023

extern int num_peers;
extern double slow_percent;
extern double low_cpu_percent;
extern long double transaction_mean_time;
extern long double block_mean_time;
extern long double current_time;
extern long int txn_counter;
extern long int blk_counter;
extern random_device rd;
extern mt19937 gen;
extern unordered_map<int,unordered_map<int,int>> rhos;

class Transaction{
    public:
    long int payer_id,receiver_id,num_coins,txn_id;
    Transaction(long int txn_id,long int payer_id,long int receiver_id,long int num_coins);
   
};

class Block;

class Event{
    public:
        string event_type;
        ld timestamp;
        Transaction* txn;
        Block* blk;
        int sender;
        int receiver;
        
        Event(string event_type,ld timestamp,Transaction*txn=0, int sender=-1);
        void process_event();
};

inline auto comp = [](Event* a,Event* b)->bool{
    return a->timestamp < b->timestamp;
};

class Node{
    public:
        bool is_malicious;
        long int node_id;
        bool is_slow,is_low_cpu;
        Event* latest_mining_event;
        long double hash_power;
        long int total_blocks;
        ofstream outFile;

        unordered_map<long int,unordered_set<long int>> blockchain_tree;       // tree[parent] = child
        unordered_map<long int,Block*> blk_id_to_pointer;       // 1 is the block id of genesis block and 0 denotes null block
        set<pair<Block*,long double>> orphaned_blocks;
        Block* longest_chain_leaf;
        unordered_set<int> neighbours,overlay_neighbours;
        unordered_map<long int,Transaction*> mempool;
        vector<long int>balances;

        Node(long int node_id, bool is_slow,bool);
        Transaction* generate_transaction();     
        Event* generate_trans_event();
        Event* generate_block_event(long int id=-1);
        bool update_tree_and_add(Block*b,Block*prev_block,bool del_lat_mining_event = true);
        bool check_balance_validity(Block*);
        void print_tree_to_file();
        bool traverse_to_genesis_and_check(Block*,bool);
        void remove_txns_from_mempool(Block*);
        void print_stats(ofstream&);
        unordered_set<int> get_neighbours(bool);
};

class Block{
    public:
        long int blk_id;
        long int prev_blk_id;
        vector<Transaction*> transactions;
        long int block_size=TXN_SIZE;        // bits
        long int depth=0;         // placeholder value
        long double timestamp=0;
        long int miner=-1;       // Node_id of the miner
};

extern unordered_map<int,Node*> nodes;
extern set<Event*,decltype(comp)> events;
extern vector<long int> malicious_node_list;
extern Node* ringmaster;
#endif

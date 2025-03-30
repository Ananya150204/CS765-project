#ifndef dec_H
#define dec_H
#include<bits/stdc++.h>
#include "argparse.h"
using namespace std;
typedef long double ld;
#define TXN_SIZE 8192    // in bits
#define MAX_BLK_SIZE 8388608    // in bits
#define MAX_TXNS 1023
#define HASH_SIZE 512        // in bits

extern int num_peers;
extern double slow_percent;
extern long double transaction_mean_time;
extern long double block_mean_time;
extern long double current_time;
extern long int txn_counter;
extern long int blk_counter;
extern random_device rd;
extern mt19937 gen;
extern unordered_map<int,unordered_map<int,int>> rhos,rhos_overlay;
extern double timeout;
extern bool no_eclipse_attack;

class Transaction{
    public:
    long int payer_id,receiver_id,num_coins,txn_id;
    bool is_get = false;
    Transaction(long int txn_id,long int payer_id,long int receiver_id,long int num_coins);
};

class Block;
class GET_REQ;

class Event{
    public:
        bool sent_on_overlay=false;
        GET_REQ* get_req = NULL;
        string event_type;
        ld timestamp;
        Transaction* txn = NULL;
        Block* blk = NULL;
        int msg_size;
        size_t hash;
        int sender;
        int receiver;
        
        Event(string event_type,ld timestamp,Transaction*txn=0, int sender=-1);
        void process_event();
};

inline auto comp = [](Event* a,Event* b)->bool{
    if(a->timestamp != b->timestamp)
        return a->timestamp < b->timestamp;
    return a<b;
};

class Node{
    public:
        bool is_malicious;
        long int node_id;
        bool is_slow;
        Event* latest_mining_event = NULL;
        
        long double hash_power;
        long int total_blocks;
        ofstream outFile;

        unordered_map<long int,unordered_set<long int>> blockchain_tree;       // tree[parent] = child
        unordered_map<long int,Block*> blk_id_to_pointer;       // 1 is the block id of genesis block and 0 denotes null block
        map<Block*,long double> orphaned_blocks;
        Block* longest_chain_leaf;
        unordered_set<int> neighbours;
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

        unordered_map<size_t,queue<pair<int,bool>>> pot_blk_senders;
        unordered_map<size_t,pair<GET_REQ*,Event*>> sent_get_requests;
        unordered_map<size_t,Block*> hash_to_block;
};

class Malicious_Node:public Node{
    public:
    bool attack_enabled = false;
    unordered_set<int> overlay_neighbours;
    vector<Block*> private_chain;
    Block* private_chain_leaf;
    vector<long int> private_balances;
    map<Block*,long double> private_orphaned_blocks;

    Malicious_Node(long int node_id, bool is_slow,bool is_malicious);
    bool check_private_block(Block*);
    void forward_broad_pvt_chain_msg(int event_sender=-1);
    void forward_stop_atk(int event_sender = -1);
};

class Block{
    public:
        long int blk_id;
        long int prev_blk_id;
        vector<Transaction*> transactions;
        long int block_size=TXN_SIZE+HASH_SIZE;        // bits
        long int depth=0;         // placeholder value
        long double timestamp=0;
        long int miner=-1;       // Node_id of the miner
        
        size_t getHash() const {
            size_t hash_val = 0;
            hash_val ^= std::hash<long int>{}(blk_id) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            hash_val ^= std::hash<long int>{}(prev_blk_id) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            hash_val ^= std::hash<long int>{}(block_size) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            hash_val ^= std::hash<long int>{}(depth) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            hash_val ^= std::hash<long double>{}(timestamp) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            hash_val ^= std::hash<long int>{}(miner) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            // Optional: hash transaction pointers/IDs if needed
            return hash_val;
        }
};

class GET_REQ{
    public:
    Event* timeout_event = NULL;
    int sender,receiver;
    size_t hash;
    GET_REQ(int sender,int receiver,size_t hash){
        this->sender = sender;
        this->receiver = receiver;
        this->hash = hash;
    }
};

extern unordered_map<int,Node*> nodes;
extern set<Event*,decltype(comp)> events;
extern vector<long int> malicious_node_list;
extern Malicious_Node* ringmaster;
extern Block* genesis_block;

long int draw_from_uniform(long int low,long int high);
double draw_from_exp(double lambda);
double draw_from_uniform_cont(double low, double high);

unordered_set<int>* get_neighbours(Node*n,bool overlay);
long double find_travelling_time(int i,int j,int msg_size,bool overlay = false);
#endif



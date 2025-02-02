#ifndef dec_H
#define dec_H
#include<bits/stdc++.h>
using namespace std;
typedef long double ld;
#define TXN_SIZE 8192

class Transaction{
    public:
    long int payer_id,receiver_id,num_coins,txn_id;
    Transaction(long int txn_id,long int payer_id,long int receiver_id,long int num_coins);
   
};

class Event{
    public:
        string event_type;
        ld timestamp;
        Transaction* txn;
        int sender;
        int receiver;
        
        Event(string event_type,ld timestamp,Transaction*txn, int sender);
        void process_event();
};

inline auto comp = [](Event* a,Event* b)->bool{
    return a->timestamp > b->timestamp;
};

class Node{
    public:
        long int node_id,coins_owned;
        bool is_slow,is_low_cpu;
        long double last_gen_time;      // microseconds
        unordered_set<int> neighbours;
        unordered_map<long int,Transaction*> mempool;
        Node(long int node_id, bool is_slow,bool is_low_cpu,long int coins_owned);
        Transaction* generate_transaction();     
        Event* generate_trans_event();
};

extern int num_peers;
extern double slow_percent;
extern double low_cpu_percent;
extern double transaction_lambda;
extern double block_lambda;
extern long double current_time;
extern long int txn_counter;
extern random_device rd;
extern mt19937 gen;
extern unordered_map<int,unordered_map<int,int>> rhos;

extern unordered_map<int,Node*> nodes;
extern priority_queue<Event*,vector<Event*>,decltype(comp)> events;

#endif
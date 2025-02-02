#include<bits/stdc++.h>
#ifndef b
#include "transaction.cpp"
#include "simulator.cpp"
#include "event.cpp"
#endif 
using namespace std;

class Node{
    public:
        long int node_id,coins_owned;
        bool is_slow,is_low_cpu;
        Node(long int node_id, bool is_slow,bool is_low_cpu){
            this->node_id = node_id;
            this->is_slow = is_slow;
            this->is_low_cpu = is_low_cpu;
        }
        Transaction* generate_transaction(){            
            uniform_int_distribution<> dist(1, num_peers);
            uniform_int_distribution<> dist2(1, this->coins_owned);
    
            Transaction* t = new Transaction(txn_counter,this->node_id,dist(gen),dist2(gen));
            txn_counter++;
            return t;
        }
        Event* generate_event(string event_type){
            Transaction* t = generate_transaction();
            exponential_distribution<> exp_dist(transaction_lambda);          
            double value = exp_dist(gen);
            Event *e = new Event(event_type,value+current_time,t);
            return e;
        }
};
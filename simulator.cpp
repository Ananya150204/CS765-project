#include<bits/stdc++.h>
#include <cstdlib> 
using namespace std;
typedef long double ld;

int num_peers=10;
double slow_percent=20;
double low_cpu_percent=30;
double transaction_lambda=30;
double block_lambda=600;
long double current_time=0;
long int txn_counter = 1;
random_device rd;
mt19937 gen(rd());

class Transaction{
    public:
    long int payer_id,receiver_id,num_coins,txn_id;
    Transaction(long int txn_id,long int payer_id,long int receiver_id,long int num_coins){
        this->txn_id = txn_id;
        this->payer_id = payer_id;
        this->receiver_id = receiver_id;
        this->num_coins = num_coins;
    }
};

class Event{
    public:
        string event_type;
        ld timestamp;
        Transaction* txn;
        
        Event(string event_type,ld timestamp,Transaction*txn){
            this->event_type = event_type;
            this->timestamp = timestamp;
            this->txn = txn;
        }
};

class Node{
    public:
        long int node_id,coins_owned;
        bool is_slow,is_low_cpu;
        Node(long int node_id, bool is_slow,bool is_low_cpu,int coins_owned){
            this->node_id = node_id;
            this->is_slow = is_slow;
            this->is_low_cpu = is_low_cpu;
            this->coins_owned = coins_owned;
        }
        Transaction* generate_transaction(){            
            uniform_int_distribution<> dist(1, num_peers);
            long int val = dist(gen);
            if(val==this->node_id){
                val = val % num_peers + 1;
            }
            uniform_int_distribution<> dist2(1, this->coins_owned);
    
            Transaction* t = new Transaction(txn_counter,this->node_id,val,dist2(gen));
            
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

auto comp = [](Event*&a,Event*&b)->bool{
    return a->timestamp > b->timestamp;
};

vector<Node*> nodes;
priority_queue<Event*,vector<Event*>,decltype(comp)> events(comp);

void generate_nodes(){
    for(int i=0;i<num_peers;i++){
        if(i<slow_percent*num_peers/100) nodes.push_back(new Node(i+1,true,false,2*i+1));
        else nodes.push_back(new Node(i+1,false,false,2*i+1));
    }
    shuffle(nodes.begin(),nodes.end(),gen);
    for(int i=0;i<low_cpu_percent*num_peers/100;i++){
        nodes[i]->is_low_cpu = true;
    }
}
void generate_events(){
    for(int i=0;i<num_peers;i++){
        events.push(nodes[i]->generate_event("gen_trans"));
    }
}

void run_events(){
    while(!events.empty()){
        Event* e = events.top();
        events.pop();
        cout << e->txn->payer_id <<  " generated a transaction at time " << e->timestamp << " with id " << e->txn->txn_id << endl;
    }
}

int main(int argc, char* argv[]) {
    generate_nodes();
    generate_events();
    run_events();
    return 0;
}
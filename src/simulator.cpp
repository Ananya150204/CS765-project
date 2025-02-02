#include "declarations.h"

int num_peers=10;
double slow_percent=20;
double low_cpu_percent=30;
double transaction_lambda=30;
double block_lambda=600;
long double current_time=0;
long int txn_counter = 1;

vector<Node*> nodes;
priority_queue<Event*,vector<Event*>,decltype(comp)> events(comp);
random_device rd;
mt19937 gen(rd());

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
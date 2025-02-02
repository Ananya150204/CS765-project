#include "declarations.h"

// gen_trans, rec_trans, for_trans, broad_mined_block, for_block, rec_block

Event::Event(string event_type,ld timestamp,Transaction*txn,int sender){
    this->event_type = event_type;
    this->timestamp = timestamp;
    this->txn = txn;
    this->sender = sender;
}

long double find_travelling_time(int i,int j){
    long double travelling_time = rhos[i][j]*1000;
    long double c_i_j = 100;
    if(nodes[i]->is_slow || nodes[j]->is_slow){
        c_i_j = 5;   
    }
    travelling_time += TXN_SIZE/c_i_j;
    exponential_distribution<> dis(96000/c_i_j);
    travelling_time += dis(gen); //d_i_j
    return travelling_time;
}

void Event::process_event(){
    if(this->event_type == "gen_trans"){
        nodes[this->sender]->mempool.insert({this->txn->txn_id,this->txn});

        // ofstream outFile("outputs/" + to_string(this->sender) + ".txt",ios::app);
        // outFile << this->timestamp << " generated " << " txn_id " << this->txn->txn_id << endl;
        // outFile.close();

        for(int j:nodes[this->sender]->neighbours){
            long double travelling_time = find_travelling_time(this->sender,j);
            Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->sender);
            e->receiver = j;
            events.push(e);
        }
    }
    else if(this->event_type == "rec_trans"){
        if(nodes[this->receiver]->mempool.find(this->txn->txn_id) != nodes[this->receiver]->mempool.end()){
            return;
        } 

        nodes[this->receiver]->mempool.insert({this->txn->txn_id,this->txn});

        // ofstream outFile("outputs/" + to_string(this->receiver) + ".txt",ios::app);
        // outFile << this->timestamp << " received by " << this->sender << " with txn_id " << this->txn->txn_id << endl;
        // outFile.close();

        int event_sender = this->sender;
        for(int j:nodes[sender]->neighbours){
            if(j==event_sender) continue;
            long double travelling_time = find_travelling_time(this->receiver,j);
            Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->receiver);
            e->receiver = j;
            events.push(e);
        }
    }
}
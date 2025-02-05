#include "declarations.h"

// gen_trans, rec_trans, for_trans, broad_mined_block, for_block, rec_block

Event::Event(string event_type,ld timestamp,Transaction*txn,int sender){
    this->event_type = event_type;
    this->timestamp = timestamp;
    this->txn = txn;
    this->sender = sender;
}

long double find_travelling_time(int i,int j,int msg_size){
    long double travelling_time = rhos[i][j]*1000;
    long double c_i_j = 100;
    if(nodes[i]->is_slow || nodes[j]->is_slow){
        c_i_j = 5;   
    }
    travelling_time += msg_size/c_i_j;
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
            long double travelling_time = find_travelling_time(this->sender,j,TXN_SIZE);
            Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->sender);
            e->receiver = j;
            events.insert(e);
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
            long double travelling_time = find_travelling_time(this->receiver,j,TXN_SIZE);
            Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->receiver);
            e->receiver = j;
            events.insert(e);
        }
    }
    else if(this->event_type == "gen_block"){
        Node* cur_node = nodes[this->sender];
        Block* prev_block = cur_node->blk_id_to_pointer[this->blk->prev_blk_id];

        cur_node->blk_id_to_pointer[this->blk->blk_id] = this->blk;
        cur_node->blockchain_tree[this->blk->prev_blk_id].insert(this->blk->blk_id);

        this->blk->depth = 1 + prev_block->depth;

        // TODO: for ties
        // This if will always be true since blocks are always mined on the longest chain
        // If somehow the chain breaks before then this event would be cancelled and would not have taken place
        if(cur_node->longest_chain_leaf->depth < this->blk->depth){
            cur_node->longest_chain_leaf = this->blk;
            cur_node->last_blk_rec_time = this->timestamp;
        }

        for(int j:cur_node->neighbours){
            long double travelling_time = find_travelling_time(this->sender,j,this->blk->block_size);
            Event* e = new Event("rec_block",this->timestamp+travelling_time);
            e->sender = this->sender;
            e->blk = this->blk;

            e->receiver = j;
            events.insert(e);
        }
    }
    else if(this->event_type == "rec_block"){
        // TODO: validate block
        // If not valid return
        // cout << "Entered at rec_block for receiver "  << this->receiver << " at timestamp " << this->timestamp << endl;        

        // If block already there in the tree of receiver (of this event) then eat five star
        if(nodes[this->receiver]->blk_id_to_pointer.find(this->blk->blk_id) != 
        nodes[this->receiver]->blk_id_to_pointer.end()){
            return;
        } 

        // 
        //
        
        Node* cur_node = nodes[this->receiver];
        if(cur_node->blk_id_to_pointer[this->blk->prev_blk_id]==NULL){
            cout <<  "Ye nahi ana chahye tha" << endl;
        }
        Block* prev_block = cur_node->blk_id_to_pointer[this->blk->prev_blk_id];
        this->blk->depth = 1 + prev_block->depth;

        cur_node->blk_id_to_pointer[this->blk->blk_id] = this->blk;
        cur_node->blockchain_tree[this->blk->prev_blk_id].insert(this->blk->blk_id);

        // TODO: for ties
        if(cur_node->longest_chain_leaf->depth < this->blk->depth){
            cur_node->longest_chain_leaf = this->blk;
            cur_node->last_blk_rec_time = this->timestamp;  
            if(cur_node->latest_mining_event){
                events.erase(cur_node->latest_mining_event);
                cout << "Mining stopped" << endl;
                long int cancel_hone_wala_id = cur_node->latest_mining_event->blk->blk_id;
                events.insert(cur_node->generate_block_event(cancel_hone_wala_id));
            }
        }

        for(int j:cur_node->neighbours){
            if(j==this->sender) continue;
            long double travelling_time = find_travelling_time(this->sender,j,this->blk->block_size);
            Event* e = new Event("rec_block",this->timestamp+travelling_time);
            e->sender = this->sender;
            e->blk = this->blk;

            e->receiver = j;
            events.insert(e);
        }
    }
}
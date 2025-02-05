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

void forward_blocks(Node*cur_node,Block*b,long int event_sender){
    for(int j:cur_node->neighbours){
        if(j==event_sender) continue;
        long double travelling_time = find_travelling_time(cur_node->node_id,j,b->block_size);
        Event* e = new Event("rec_block",current_time+travelling_time);
        e->sender = cur_node->node_id;
        e->blk = b;

        e->receiver = j;
        events.insert(e);
    }
}

void Event::process_event(){
    if(this->event_type == "gen_trans"){
        nodes[this->sender]->mempool.insert({this->txn->txn_id,this->txn});

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

        cur_node->update_tree_and_add(this->blk,prev_block);

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
        Node* cur_node = nodes[this->receiver];
        if(!cur_node->check_balance_validity(this)) return;

        // If block already there in the tree of receiver (of this event) then eat five star
        if(cur_node->blk_id_to_pointer.find(this->blk->blk_id) !=  cur_node->blk_id_to_pointer.end()) return; 

        if(cur_node->blk_id_to_pointer[this->blk->prev_blk_id]==NULL){
            cur_node->orphaned_blocks.insert(this->blk);
            return;
        }

        Block* prev_block = cur_node->blk_id_to_pointer[this->blk->prev_blk_id];
        cur_node->update_tree_and_add(this->blk,prev_block);

        for(auto b:cur_node->orphaned_blocks){
            if(cur_node->blk_id_to_pointer[b->prev_blk_id]){
                cur_node->update_tree_and_add(b,cur_node->blk_id_to_pointer[b->prev_blk_id]);
                cur_node->orphaned_blocks.erase(b);
            }
            forward_blocks(cur_node,b,-1);
            // We are ignoring bina baap ke baccho wale events ke sender as of now 
            // the receiving block will take care 
            // If required, we will add later
        }

        forward_blocks(cur_node,this->blk,this->sender);
    }
}
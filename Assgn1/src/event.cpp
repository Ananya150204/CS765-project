#include "declarations.h"

Event::Event(string event_type,ld timestamp,Transaction*txn,int sender){
    this->event_type = event_type;
    this->timestamp = timestamp;
    this->txn = txn;
    this->sender = sender;
}

// Calculates the time taken to travel from node i to node j given a particular
// message size
long double find_travelling_time(int i,int j,int msg_size){
    long double travelling_time = rhos[i][j]*1000;
    long double c_i_j = 100;
    if(nodes[i]->is_slow || nodes[j]->is_slow){
        c_i_j = 5;   
    }
    travelling_time += msg_size/c_i_j;
    exponential_distribution<> dis(96000/c_i_j);
    travelling_time += dis(gen); // d_i_j sampled from exp distribution
    return travelling_time;
}

// Forwards the block to all its immediate neighbors in the graph
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


// Deals with the various types of event as :
// gen_trans, rec_trans, gen_block, rec_block
void Event::process_event(){
    if(this->event_type == "gen_trans"){
        if(this->txn->num_coins==0) return;

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
        if(this->txn->num_coins==0) return;

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
        if(!cur_node->check_balance_validity(this->blk)) return;
        Block* prev_block = cur_node->blk_id_to_pointer[this->blk->prev_blk_id];

        if(!cur_node->update_tree_and_add(this->blk,prev_block,false)) return;
        cur_node->total_blocks++;
        cur_node->outFile << this->blk->blk_id << "," << this->blk->prev_blk_id << "," << current_time << "," << current_time << endl;

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
        // If not valid, return.
        Node* cur_node = nodes[this->receiver];
        if(!cur_node->check_balance_validity(this->blk)) return;

        // If block already there in the tree of receiver (of this event) then do nothing.
        if(cur_node->blk_id_to_pointer.find(this->blk->blk_id) !=  cur_node->blk_id_to_pointer.end()) return; 

        if(cur_node->blk_id_to_pointer[this->blk->prev_blk_id]==NULL){
            cur_node->orphaned_blocks.insert({this->blk,current_time});
            forward_blocks(cur_node,this->blk,this->sender);
            return;
        }

        Block* prev_block = cur_node->blk_id_to_pointer[this->blk->prev_blk_id];
        if(!cur_node->update_tree_and_add(this->blk,prev_block)) return;
        cur_node->outFile << this->blk->blk_id << "," << this->blk->prev_blk_id << "," << current_time << "," << current_time << "," << this->blk->block_size/TXN_SIZE << endl;

        for(auto& [b, t]:cur_node->orphaned_blocks){
            if(cur_node->blk_id_to_pointer[b->prev_blk_id]){
                cur_node->check_balance_validity(b);
                if(cur_node->update_tree_and_add(b,cur_node->blk_id_to_pointer[b->prev_blk_id])){
                    cur_node->outFile << b->blk_id << "," << b->prev_blk_id << "," << t << "," << current_time << endl;
                }
                cur_node->orphaned_blocks.erase({b,t});
            }
            // Forwarding orphaned block here is not required since we are forwarding every valid block when it is received.
        }

        forward_blocks(cur_node,this->blk,this->sender);
    }
}

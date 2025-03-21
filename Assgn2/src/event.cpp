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

// Forwards the "hash" of the block to all its immediate neighbors in the graph
void forward_hash(Node*cur_node,Block*b,long int event_sender){
    for(int j:cur_node->neighbours){
        if(j==event_sender) continue;
        long double travelling_time = find_travelling_time(cur_node->node_id,j,TXN_SIZE);
        Event* e = new Event("rec_hash",current_time+travelling_time);
        e->sender = cur_node->node_id;
        e->hash = b->getHash();

        e->receiver = j;
        events.insert(e);
    }
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

void forward_get_req(Node*cur_node,size_t hash,GET_REQ* get_req){
    int tobe = cur_node->pot_blk_senders[hash].front();
    cur_node->pot_blk_senders[hash].pop();
    long double travelling_time = find_travelling_time(cur_node->node_id,tobe,TXN_SIZE);
    Event* e = new Event("rec_get_req",current_time+travelling_time);
    e->get_req = get_req;
    e->sender = cur_node->node_id;
    e->receiver = tobe;
    e->hash = hash;
    events.insert(e);
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

        cur_node->hash_to_block[this->blk->getHash()] = this->blk;
        // forward_blocks(nodes[this->sender],this->blk,this->sender);
        forward_hash(nodes[this->sender],this->blk,this->sender);
    }
    else if(this->event_type == "rec_hash"){
        Node* cur_node = nodes[this->receiver];
        cur_node->pot_blk_senders[this->hash].push(this->sender);

        if(!cur_node->valid_get_requests.contains(this->hash)){
            GET_REQ* get_req = new GET_REQ(this->receiver,this->sender,this->hash);
            get_req->timeout_event = new Event("timeout",current_time+timeout);
            get_req->timeout_event->receiver = this->receiver;
            get_req->hash = this->hash;
            events.insert(get_req->timeout_event);
            cur_node->valid_get_requests[this->hash] = get_req;
            forward_get_req(cur_node,this->hash,get_req);
        }
    }
    else if(this->event_type == "rec_get_req"){
        Node* cur_node = nodes[this->receiver];
        if(cur_node->hash_to_block.contains(this->hash)){
            Block* b = cur_node->hash_to_block[this->hash];
            long double travelling_time = find_travelling_time(this->receiver,this->sender,b->block_size);
            Event* e = new Event("rec_block",current_time+travelling_time);
            e->sender = this->receiver;
            e->blk = b;
            e->receiver = this->sender;
            e->get_req = this->get_req;
            e->hash = hash;
            events.insert(e);
        }
    }
    else if(this->event_type == "timeout"){ // timeout period is over
        Node* cur_node = nodes[this->receiver];
        cur_node->valid_get_requests.erase(this->hash);
        if(cur_node->pot_blk_senders.contains(this->hash) && cur_node->pot_blk_senders[this->hash].size()>0){
            int tobe = cur_node->pot_blk_senders[this->hash].front();

            GET_REQ* get_req = new GET_REQ(tobe,this->sender,this->hash);
            get_req->timeout_event = new Event("timeout",current_time+timeout);
            events.insert(get_req->timeout_event);
            cur_node->valid_get_requests[this->hash] = get_req;
            forward_get_req(cur_node,this->hash,get_req);
        }
    }
    else if(this->event_type == "rec_block"){
        // If not valid, return.
        Node* cur_node = nodes[this->receiver];
        if(!cur_node->valid_get_requests.contains(this->hash) || cur_node->valid_get_requests[this->hash]!=this->get_req) return;
        events.erase(this->get_req->timeout_event);
        delete this->get_req;
        cur_node->valid_get_requests.erase(this->hash);
        cur_node->pot_blk_senders.erase(this->hash);

        if(!cur_node->check_balance_validity(this->blk)) return;

        // If block already there in the tree of receiver (of this event) then do nothing.
        if(cur_node->blk_id_to_pointer.find(this->blk->blk_id) !=  cur_node->blk_id_to_pointer.end()) return; 

        if(!cur_node->blk_id_to_pointer.contains(this->blk->prev_blk_id)){
            cur_node->orphaned_blocks.insert({this->blk,current_time});
            // forward_blocks(cur_node,this->blk,this->sender);
            forward_hash(cur_node,this->blk,this->sender);
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

        // forward_blocks(cur_node,this->blk,this->sender);
        forward_hash(cur_node,this->blk,this->sender);

    }
}

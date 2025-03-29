#include "declarations.h"

Event::Event(string event_type,ld timestamp,Transaction*txn,int sender){
    this->event_type = event_type;
    this->timestamp = timestamp;
    this->txn = txn;
    this->sender = sender;
}

// Calculates the time taken to travel from node i to node j given a particular
// message size
long double find_travelling_time(int i,int j,int msg_size,bool overlay){
    long double travelling_time = rhos[i][j]*1000;
    if(overlay) travelling_time = rhos_overlay[i][j]*1000;
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
void forward_hash(Node*cur_node,Block*b,long int event_sender,bool overlay = false){
    if(overlay && !cur_node->is_malicious) {cerr << "Mishap happened" << endl;exit(1);}
    unordered_set<int>* neigh = get_neighbours(cur_node,overlay);

    for(int j:*(neigh)){
        if(j==event_sender) continue;
        // if sending on honest chain(!overlay) and the receiver is a neighbour in the overlay network also then skip
        if(!overlay && cur_node->is_malicious && get_neighbours(cur_node,true)->find(j) != get_neighbours(cur_node,true)->end()) continue;

        long double travelling_time = find_travelling_time(cur_node->node_id,j,TXN_SIZE,overlay);
        Event* e = new Event("rec_hash",current_time+travelling_time);
        e->sender = cur_node->node_id;
        e->hash = b->getHash();
        e->sent_on_overlay = overlay;
        e->receiver = j;
        events.insert(e);
    }
}


void forward_get_req(Node*cur_node,size_t hash,GET_REQ* get_r){
    if(!cur_node->pot_blk_senders.contains(hash)) {cerr << "missing hash" << endl;exit(1);}
    auto tmp = cur_node->pot_blk_senders[hash].front();
    int tobe = tmp.first;
    bool sent_on_overlay = tmp.second;
    cur_node->pot_blk_senders[hash].pop();
    long double travelling_time = find_travelling_time(cur_node->node_id,tobe,TXN_SIZE,sent_on_overlay);
    Event* e = new Event("rec_get_req",current_time+travelling_time);
    e->sent_on_overlay = sent_on_overlay;
    e->get_req = get_r;
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

        unordered_set<int>* neigh = get_neighbours(nodes[this->sender],false);
        for(int j:*neigh){
            long double travelling_time = find_travelling_time(this->sender,j,TXN_SIZE);
            Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->sender);
            e->receiver = j;
            events.insert(e);
        }
        if(nodes[this->sender]->is_malicious){
            neigh = get_neighbours(nodes[this->sender],true);
            for(int j:*neigh){
                long double travelling_time = find_travelling_time(this->sender,j,TXN_SIZE,true);
                Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->sender);
                e->receiver = j;
                events.insert(e);
            }
        }
    }
    else if(this->event_type == "rec_trans"){
        Node* cur_node = nodes[this->receiver];
        if(this->sent_on_overlay && !cur_node->is_malicious){cerr << "Mishap happened" << endl;exit(1);}
        if(cur_node->mempool.find(this->txn->txn_id) != cur_node->mempool.end()){
            return;
        }
        if(this->txn->num_coins==0) return;

        cur_node->mempool.insert({this->txn->txn_id,this->txn});

        int event_sender = this->sender;
        unordered_set<int>* neigh = get_neighbours(cur_node,false);
        for(int j:*neigh){
            if(j==event_sender) continue;

            long double travelling_time = find_travelling_time(this->receiver,j,TXN_SIZE);
            Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->receiver);
            e->receiver = j;
            events.insert(e);
        }
        if(cur_node->is_malicious){
            neigh = get_neighbours(cur_node,true);
            for(int j:*neigh){
                if(j==event_sender) continue;

                long double travelling_time = find_travelling_time(this->receiver,j,TXN_SIZE,true);
                Event* e = new Event("rec_trans",this->timestamp+travelling_time,this->txn,this->receiver);
                e->receiver = j;
                events.insert(e);
            }
        }
    } 
    else if(this->event_type == "gen_block"){
        Node* cur_node = nodes[this->sender];
        if(this->sent_on_overlay && !cur_node->is_malicious){cerr << "Mishap happened" << endl;exit(1);}
        if(ringmaster->node_id == cur_node->node_id){ 
            Block*b = this->blk;
            Malicious_Node* current_node = (Malicious_Node*)cur_node;
            if(!current_node->check_private_block(b)) {cerr << "private invalidated" << endl;return;}
            Block*prev_block = current_node->blk_id_to_pointer[this->blk->prev_blk_id];
            current_node->blk_id_to_pointer[b->blk_id] = b;
            current_node->blockchain_tree[b->prev_blk_id].insert(b->blk_id);
            current_node->private_chain.push_back(b);

            current_node->total_blocks++;
            current_node->outFile << this->blk->blk_id << "," << this->blk->prev_blk_id << "," << current_time << "," << current_time << "," << this->blk->block_size/TXN_SIZE << "," << nodes[this->blk->miner]->is_malicious << endl;

            if(current_node->private_chain_leaf->depth < b->depth){
                current_node->private_chain_leaf = b;
                current_node->remove_txns_from_mempool(b);
            }
            cur_node->hash_to_block[this->blk->getHash()] = this->blk;
            forward_hash(nodes[this->sender],this->blk,this->sender,true);
        }
        else{   // honest and other malicious waalo ka hisaab
            if(!cur_node->check_balance_validity(this->blk)) {cerr << "invalid block in gen block" << endl;return;}
            Block* prev_block = cur_node->blk_id_to_pointer[this->blk->prev_blk_id];
            if(!cur_node->update_tree_and_add(this->blk,prev_block,false)) return;

            cur_node->total_blocks++;
            cur_node->outFile << this->blk->blk_id << "," << this->blk->prev_blk_id << "," << current_time << "," << current_time << "," << this->blk->block_size/TXN_SIZE << "," << nodes[this->blk->miner]->is_malicious << endl;

            cur_node->hash_to_block[this->blk->getHash()] = this->blk;
            forward_hash(nodes[this->sender],this->blk,this->sender);
        }
    }
    else if(this->event_type == "rec_hash"){
        Node* cur_node = nodes[this->receiver];
        if(this->sent_on_overlay && !cur_node->is_malicious){cerr << "Mishap happened" << endl;exit(1);}

        if(cur_node->hash_to_block.contains(this->hash)) return;
        cur_node->pot_blk_senders[this->hash].push({this->sender,this->sent_on_overlay});

        if(!cur_node->sent_get_requests.contains(this->hash)){
            GET_REQ* get_req = new GET_REQ(this->receiver,this->sender,this->hash);
            Event* tm = new Event("timeout",current_time+timeout);
            tm->receiver = this->receiver;
            tm->hash = this->hash;
            events.insert(tm);
            cur_node->sent_get_requests[this->hash] = {get_req,tm};
            forward_get_req(cur_node,this->hash,get_req);
        }
    }
    else if(this->event_type == "rec_get_req"){
        Node* cur_node = nodes[this->receiver];
        if(this->sent_on_overlay && !cur_node->is_malicious){cerr << "Mishap happened" << endl;exit(1);}
        
        bool sent_on_overlay = this->sent_on_overlay;
        bool is_ring_block = cur_node->hash_to_block[this->hash]->miner == ringmaster->node_id;
        if(cur_node->is_malicious && !no_eclipse_attack && !nodes[this->sender]->is_malicious && !is_ring_block) {return;}

        if(cur_node->hash_to_block.contains(this->hash)){
            Block* b = cur_node->hash_to_block[this->hash];
            long double travelling_time = find_travelling_time(this->receiver,this->sender,b->block_size,sent_on_overlay);
            Event* e = new Event("rec_block",current_time+travelling_time);
            e->sent_on_overlay = sent_on_overlay;
            e->sender = this->receiver;
            e->blk = b;
            e->receiver = this->sender;
            e->get_req = this->get_req;
            e->hash = this->hash;
            events.insert(e);
        }
    }
    else if(this->event_type == "timeout"){ // timeout period is over
        Node* cur_node = nodes[this->receiver];

        if(!cur_node->sent_get_requests.contains(this->hash)) {cerr << "hash " << this->hash << " not found in timeout" << endl;exit(1);}
        cur_node->sent_get_requests.erase(this->hash);

        if(cur_node->pot_blk_senders.contains(this->hash) && cur_node->pot_blk_senders[this->hash].size()>0){
            auto tmp = cur_node->pot_blk_senders[this->hash].front();
            int tobe = tmp.first;

            GET_REQ* get_req = new GET_REQ(this->receiver,tobe,this->hash);
            Event* tm = new Event("timeout",current_time+timeout);
            tm->receiver = this->receiver;
            tm->hash = this->hash;
            events.insert(tm);
            cur_node->sent_get_requests[this->hash] = {get_req,tm};
            forward_get_req(cur_node,this->hash,get_req);
        }
    }
    else if(this->event_type == "broadcast private chain"){
        Malicious_Node* cur_node = (Malicious_Node*)nodes[this->receiver];
        cur_node->attack_enabled = true;
        if(cur_node->private_chain.size()==0) return;
        
        if(this->sent_on_overlay && !cur_node->is_malicious){cerr << "Mishap happened" << endl;exit(1);}
        
        cur_node->forward_broad_pvt_chain_msg(this->sender);
        unordered_set<int>* neigh = get_neighbours(cur_node,false);
        for(int j:*neigh){
            if(nodes[j]->is_malicious) continue;
            for(Block*b:cur_node->private_chain){
                forward_hash(cur_node,b,j);
            }
        }
        cur_node->private_chain = vector<Block*>();   
    }
    else if(this->event_type == "rec_block"){
        // If not valid, return.
        Node* cur_node = nodes[this->receiver];
        if(this->sent_on_overlay && !cur_node->is_malicious){cerr << "Mishap happened" << endl;exit(1);}

        if(this->get_req->hash != this->blk->getHash()) return;

        if(cur_node->sent_get_requests.contains(this->hash)){
            events.erase(cur_node->sent_get_requests[this->hash].second);
            delete cur_node->sent_get_requests[this->hash].second;
            cur_node->sent_get_requests.erase(this->hash);
        }
        delete this->get_req;
        cur_node->pot_blk_senders.erase(this->hash);
        
        // If block already there in the tree of receiver (of this event) then do nothing.
        if(cur_node->blk_id_to_pointer.contains(this->blk->blk_id)) return; 

        if(this->sent_on_overlay && nodes[this->blk->miner]->node_id==ringmaster->node_id){
            Malicious_Node* current_node = (Malicious_Node*)cur_node;
            Block*b = this->blk;
            if(!cur_node->blk_id_to_pointer.contains(b->prev_blk_id)) {
                cerr << "orphaned private block " << endl;
            }
            Block* prev_block = cur_node->blk_id_to_pointer[b->prev_blk_id];
            if(!current_node->check_private_block(b)) {cerr << "private invalid" << endl;return;}
            current_node->blk_id_to_pointer[b->blk_id] = b;
            current_node->blockchain_tree[b->prev_blk_id].insert(b->blk_id);
            current_node->private_chain.push_back(b);

            if(current_node->private_chain_leaf->depth < b->depth){
                current_node->private_chain_leaf = b;
                current_node->remove_txns_from_mempool(b);
            }
            current_node->hash_to_block[b->getHash()] = b;
            forward_hash(current_node,b,this->sender,true);
            if(current_node->attack_enabled) forward_hash(current_node,b,this->sender,false);
        }
        else{
            Block* b = this->blk;
            if(!cur_node->check_balance_validity(b)) {cerr << "invalid block " << b->miner << endl;return;}

            if(!cur_node->blk_id_to_pointer.contains(b->prev_blk_id)){
                cur_node->orphaned_blocks[b]=current_time;
                cur_node->hash_to_block[b->getHash()] = b;
                forward_hash(cur_node,b,this->sender,this->sent_on_overlay);
                if(cur_node->is_malicious && !this->sent_on_overlay) forward_hash(cur_node,b,this->sender,true);
                return;
            }

            Block* prev_block = cur_node->blk_id_to_pointer[b->prev_blk_id];
            if(!cur_node->update_tree_and_add(b,prev_block)) {cerr << "invalid block " << b->miner << endl;return;}
            cur_node->outFile << b->blk_id << "," << b->prev_blk_id << "," << current_time << "," << current_time << "," << b->block_size/TXN_SIZE << "," << nodes[b->miner]->is_malicious << endl;

            auto it = cur_node->orphaned_blocks.begin();
            while(it!=cur_node->orphaned_blocks.end()){
                Block*b = it->first;
                long double t = it->second;
                if(cur_node->blk_id_to_pointer.contains(b->prev_blk_id)){
                    cur_node->check_balance_validity(b);
                    if(cur_node->update_tree_and_add(b,cur_node->blk_id_to_pointer[b->prev_blk_id])){
                        cur_node->outFile << b->blk_id << "," << b->prev_blk_id << "," << t << "," << current_time << "," << b->block_size/TXN_SIZE << "," << nodes[b->miner]->is_malicious << endl;
                    }
                    it = cur_node->orphaned_blocks.erase(it);
                }
                else it++;
                // Forwarding orphaned block here is not required since we are forwarding every valid block when it is received.
            }

            cur_node->hash_to_block[b->getHash()] = b;
            forward_hash(cur_node,b,this->sender,this->sent_on_overlay);
            if(cur_node->is_malicious && !this->sent_on_overlay) forward_hash(cur_node,b,this->sender,true);

            if(cur_node->is_malicious){
                Malicious_Node* m = (Malicious_Node*)cur_node;
                if(m->private_chain_leaf->depth < m->longest_chain_leaf->depth){
                    // cerr << "resetting" << endl;
                    m->private_chain_leaf = m->longest_chain_leaf;
                    m->private_balances = m->balances;
                    m->private_chain = vector<Block*>();
                    m->attack_enabled = false;
                    if(m->latest_mining_event){
                        auto iter = events.find(m->latest_mining_event);
                        if(iter!=events.end() && *iter==m->latest_mining_event) events.erase(iter);
                        long int cancel_hone_wala_id = m->latest_mining_event->blk->blk_id;
                        delete m->latest_mining_event;
                        events.insert(m->generate_block_event(cancel_hone_wala_id));
                    }
                }
                if(cur_node->node_id == ringmaster->node_id){
                    Malicious_Node* m = (Malicious_Node*)cur_node;
                    Block* temp = m->longest_chain_leaf;
                    if((m->private_chain.size()>0 && m->private_chain_leaf->depth == temp->depth) || (m->private_chain_leaf->depth == 1+temp->depth && m->private_chain_leaf->prev_blk_id!=temp->blk_id)){
                        cerr << "ATTACK!" << endl;
                        m->forward_broad_pvt_chain_msg();
                    }
                }
            }
        }
    }
}

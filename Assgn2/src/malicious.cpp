#include "declarations.h"

Malicious_Node::Malicious_Node(long int node_id, bool is_slow,bool is_malicious):Node(node_id,is_slow,is_malicious){
    this->private_chain_leaf = genesis_block;
    this->private_balances = vector<long int>(num_peers+1,0);    
}

unordered_set<int>* get_neighbours(Node*n,bool overlay){
    if(!n->is_malicious && overlay) {
        cerr << "overlay neighbours requested for an honest node" << endl;
        exit(1);
    }

    if(overlay) return &(((Malicious_Node*)n)->overlay_neighbours);
    return &(n->neighbours);
}

bool Malicious_Node::check_private_block(Block*b){
    if(b->prev_blk_id == this->private_chain_leaf->blk_id){
        vector<long int> delta = this->private_balances;
        delta[b->miner]+=50;
        for(Transaction* txn:b->transactions){
            long int payer = txn->payer_id;
            long int receiver = txn->receiver_id;
            long int num_coins = txn->num_coins;
            if(delta[payer]<num_coins){
                this->hash_to_block.erase(b->getHash());
                return false;
            }
            delta[payer] -= num_coins;
            delta[receiver] += num_coins;
        }
        for(auto i=0;i<num_peers;i++){
            this->private_balances[i+1] = delta[i+1];
        }
    }
    else {cerr << "private block forked" << endl;exit(1);}
    return true;
}

void Malicious_Node::forward_broad_pvt_chain_msg(int event_sender){
    unordered_set<int>* neigh = get_neighbours(this,true);
    for(int j:*neigh){
        if(j==event_sender) continue;
        long double travelling_time = find_travelling_time(this->node_id,j,HASH_SIZE,true);
        Event* e = new Event("broadcast private chain",current_time+travelling_time);

        e->msg_size = HASH_SIZE;
        e->sender = this->node_id;
        e->receiver = j;
        e->sent_on_overlay = true;
        events.insert(e);
    }
}
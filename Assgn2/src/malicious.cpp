#include "declarations.h"

Malicious_Node::Malicious_Node(long int node_id, bool is_slow,bool is_malicious):Node(node_id,is_slow,is_malicious){
    this->private_chain_leaf = genesis_block;
    this->private_balances = vector<long int>(num_peers+1,0);    
}

unordered_set<int>* get_neighbours(Node*n,bool overlay){
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
    return true;
}


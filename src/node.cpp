#include "declarations.h"

Node::Node(long int node_id, bool is_slow,bool is_low_cpu,long int coins_owned){
    this->node_id = node_id;
    this->is_slow = is_slow;
    this->is_low_cpu = is_low_cpu;
    this->coins_owned = coins_owned+100;
    this->last_gen_time = 0;
    
    Block* genesis_block = new Block();
    genesis_block->blk_id = 1;
    genesis_block->prev_blk_id = 0;
    genesis_block->timestamp = 0;
    genesis_block->depth = 1;
    this->blk_id_to_pointer[1] = genesis_block;
    this->longest_chain_leaf = genesis_block;
    this->last_blk_rec_time = 0;
    this->latest_mining_event = 0;
}

Transaction* Node:: generate_transaction(){            
    uniform_int_distribution<> dist(1, num_peers);
    uniform_int_distribution<> dist2(1, this->coins_owned);

    Transaction* t = new Transaction(++txn_counter,this->node_id,dist(gen),dist2(gen));
    // cout << this->node_id << " generated id " << txn_counter-1 << endl;
    return t;
}

Event* Node:: generate_trans_event(){
    Transaction* t = generate_transaction();
    
    exponential_distribution<> exp_dist(1/transaction_mean_time);          
    long double value = exp_dist(gen);
    Event *e = new Event("gen_trans",value+this->last_gen_time,t,this->node_id);
    this->last_gen_time += value;
    return e;
}

Event* Node:: generate_block_event(long int id){       // boli lag rha h mining slot ka success hua(tree me add hone) ya nahi wo yahi wala event process jab hoga tab pata chalega
    Block* to_be_mined = new Block();
    to_be_mined->miner = this->node_id;
    to_be_mined->prev_blk_id = this->longest_chain_leaf->blk_id;

    if(id==-1) to_be_mined->blk_id  = ++blk_counter;
    else to_be_mined->blk_id = id;

    exponential_distribution<> dis(1/block_mean_time);
    to_be_mined->timestamp = last_blk_rec_time + dis(gen);
    Event *e = new Event("gen_block",to_be_mined->timestamp);
    e->blk = to_be_mined;
    e->sender = this->node_id;
    this->latest_mining_event = e;
    return e;

    // TODO: include transactions here itself because they are technically specified here only
    // TODO: set block size too
}

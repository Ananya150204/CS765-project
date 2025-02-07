#include "declarations.h"

Node::Node(long int node_id, bool is_slow,bool is_low_cpu,long double hash_power){
    this->node_id = node_id;
    this->is_slow = is_slow;
    this->is_low_cpu = is_low_cpu;
    this->hash_power = hash_power;
    this->total_blocks = 0;
    
    Block* genesis_block = new Block();
    genesis_block->blk_id = 1;
    genesis_block->prev_blk_id = 0;
    genesis_block->timestamp = 0;
    genesis_block->depth = 1;
    this->blk_id_to_pointer[1] = genesis_block;
    this->longest_chain_leaf = genesis_block;
    this->latest_mining_event = 0;
    this->balances = vector<long int>(num_peers+1,0);
    this->outFile.open("outputs/block_arrivals/" + to_string(this->node_id) + ".txt",ios::out);
    this->outFile << "Block_ID" << "," << "Prev_Blk_ID" << "," << "Arrival_time" << "," << "Time_addn_to_tree" << endl;
}

// Generates transactions as per the problem specifications.
Transaction* Node:: generate_transaction(){            
    uniform_int_distribution<> dist(1, num_peers);
    Transaction* t = new Transaction(++txn_counter,this->node_id,dist(gen),0);
    if(this->balances[this->node_id]>0){
        uniform_int_distribution<> dist2(1, this->balances[this->node_id]);
        t->num_coins = dist2(gen);
    }
    return t;
}


//  Generates an event in which a transaction is created.
Event* Node:: generate_trans_event(){
    Transaction* t = generate_transaction();
    
    exponential_distribution<> exp_dist(1/transaction_mean_time);          
    long double value = exp_dist(gen);
    Event *e = new Event("gen_trans",value+current_time,t,this->node_id);
    return e;
}


// The block will be mined at T_k time after the current time. 
// Whether this block gets added to the tree will be decided at the time of
// processing this event.
Event* Node:: generate_block_event(long int id){       
    Block* to_be_mined = new Block();
    to_be_mined->miner = this->node_id;
    to_be_mined->prev_blk_id = this->longest_chain_leaf->blk_id;

    if(id==-1) to_be_mined->blk_id  = ++blk_counter;
    else to_be_mined->blk_id = id;

    exponential_distribution<> dis(this->hash_power/block_mean_time);
    to_be_mined->timestamp = current_time + dis(gen);
    Event *e = new Event("gen_block",to_be_mined->timestamp);
    e->blk = to_be_mined;
    e->sender = this->node_id;
    this->latest_mining_event = e;
    
    int counter = 0;
    vector<long int> temp = this->balances;
    for(auto it:this->mempool){
        counter++;
        if(counter > MAX_TXNS){        // Excluding coinbase
            break;
        }
        if(temp[it.second->payer_id] >= it.second->num_coins){
            to_be_mined->transactions.push_back(it.second);
            to_be_mined->block_size+=TXN_SIZE;
            temp[it.second->payer_id] -= it.second->num_coins;
            temp[it.second->receiver_id] += it.second->num_coins;
        }
    }
    return e;
}

void Node:: print_tree_to_file(){
    ofstream outFile("outputs/blockchains/" + to_string(this->node_id) + ".txt",ios::out);
    queue<long int> q;
    q.push(1);
    while(!q.empty()){
        long int curr_block = q.front();
        q.pop();
        for(auto j:this->blockchain_tree[curr_block]){
            outFile << curr_block << " " << j << endl;
            q.push(j);
        }
    }
    outFile.close();
}

// If the block is not getting added to the current longest chain,the
// function will validate the block and update the current node's balance
// if required. If the block is added to the current longest chain, we do
// not need to travel back to the genesis block since we have balances stored
// So, check_balance_validity function does the job.
bool Node:: traverse_to_genesis_and_check(Block*b,bool reset_balance){
    unordered_map<long int,long int> longest_chain;
    int curr_id = b->blk_id;
    int prev_id = b->prev_blk_id;
    while(prev_id!=0){
        longest_chain[prev_id] = curr_id;
        curr_id = prev_id;
        prev_id = this->blk_id_to_pointer[prev_id]->prev_blk_id;
    }
    curr_id = 1;
    // We do not know whether the current block is valid, so we need to
    // keep a copy of the balances while checking the block validity.
    vector<long int> delta(num_peers+1,0);
    while(longest_chain.contains(curr_id)){
        curr_id = longest_chain[curr_id];
        Block* cur_block = this->blk_id_to_pointer[curr_id];

        delta[cur_block->miner] += 50;         
        for(Transaction*txn:cur_block->transactions){
            long int payer = txn->payer_id;
            long int receiver = txn->receiver_id;
            long int num_coins = txn->num_coins;
            if(delta[payer]<num_coins){
                return false;
            }
            delta[payer] -= num_coins;
            delta[receiver] += num_coins;
        }
    }
    if(reset_balance){
        for(auto i=0;i<num_peers;i++){
            this->balances[i+1] = delta[i+1];
        }
    }
    return true;
}

void Node::remove_txns_from_mempool(Block*b){
    for(auto t:b->transactions){
        this->mempool.erase(t->txn_id);
    }
}

// Updates the blockchain tree.
// In case the block is added to the current longest chain, validity is checked
// using the check_balance_validity function which is called during event processing
// Else, travel_to_genesis_and_check has to be used which is called from here
// In case of a block receiving event on the current longest chain, 
// the node's next mining event will be cancelled.
bool Node:: update_tree_and_add(Block*b,Block*prev_block,bool del_lat_mining_event){
    this->blk_id_to_pointer[b->blk_id] = b;
    if(this->longest_chain_leaf!=prev_block){
        if(this->longest_chain_leaf->depth < 1+prev_block->depth){
            if(!this->traverse_to_genesis_and_check(b,true)) {
                this->blk_id_to_pointer.erase(b->blk_id);
                return false;
            }
        }
        else{
            if(!this->traverse_to_genesis_and_check(b,false)) {
                this->blk_id_to_pointer.erase(b->blk_id);
                return false;
            }
        }
    }

    this->blockchain_tree[b->prev_blk_id].insert(b->blk_id);
    b->depth = 1 + prev_block->depth;

    if(this->longest_chain_leaf->depth < b->depth){
        this->longest_chain_leaf = b;
        this->remove_txns_from_mempool(b);
        if(this->latest_mining_event && del_lat_mining_event){
            events.erase(this->latest_mining_event);
            long int cancel_hone_wala_id = this->latest_mining_event->blk->blk_id;
            delete this->latest_mining_event;
            events.insert(this->generate_block_event(cancel_hone_wala_id));
        }
    }
    return true;
}  

// Already explained above -:)
bool Node:: check_balance_validity(Block*b){
    if(b->prev_blk_id == this->longest_chain_leaf->blk_id){
        vector<long int> delta = this->balances;
        delta[b->miner]+=50;
        for(Transaction* txn:b->transactions){
            long int payer = txn->payer_id;
            long int receiver = txn->receiver_id;
            long int num_coins = txn->num_coins;
            if(delta[payer]<num_coins){
                return false;
            }
            delta[payer] -= num_coins;
            delta[receiver] += num_coins;
        }
        for(auto i=0;i<num_peers;i++){
            this->balances[i+1] = delta[i+1];
        }
    }
    return true;
}

void Node:: print_stats(ofstream& outFile){
    long int chain = 0;
    long int curr_id = this->longest_chain_leaf->blk_id;
    while(curr_id!=1){
        chain+= (this->blk_id_to_pointer[curr_id]->miner == this->node_id);
        curr_id = this->blk_id_to_pointer[curr_id]->prev_blk_id;
    }
    outFile << this->node_id << "," << chain << "," << this->total_blocks << "," << !this->is_low_cpu << "," << this->hash_power<< endl;
}
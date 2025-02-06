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
}

Transaction* Node:: generate_transaction(){            
    uniform_int_distribution<> dist(1, num_peers);
    Transaction* t = new Transaction(++txn_counter,this->node_id,dist(gen),0);
    if(this->balances[this->node_id]>0){
        uniform_int_distribution<> dist2(1, this->balances[this->node_id]);
        t->num_coins = dist2(gen);
    }
    return t;
}

Event* Node:: generate_trans_event(){
    Transaction* t = generate_transaction();
    
    exponential_distribution<> exp_dist(1/transaction_mean_time);          
    long double value = exp_dist(gen);
    Event *e = new Event("gen_trans",value+current_time,t,this->node_id);
    return e;
}

Event* Node:: generate_block_event(long int id){       // boli lag rha h mining slot ka success hua(tree me add hone) ya nahi wo yahi wala event process jab hoga tab pata chalega
 
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
    
    // TODO: include transactions here itself because they are technically specified here only
    // TODO: set block size too
    int counter = 0;
    for(auto it:this->mempool){
        counter++;
        if(counter > MAX_BLK_SIZE/TXN_SIZE - 1){        // Including coinbase
            break;
        }
        if(this->balances[it.second->payer_id] >= it.second->num_coins){
            to_be_mined->transactions.push_back(it.second);
            to_be_mined->block_size+=TXN_SIZE;
        }
    }
    return e;
}

void Node:: print_tree_to_file(){
    ofstream outFile("outputs/blockchains/" + to_string(this->node_id) + ".txt",ios::app);
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

bool Node:: traverse_to_genesis_and_check(Block*b){
    this->balances = vector<long int>(1+num_peers,0);
    unordered_map<long int,long int> longest_chain;
    int curr_id = b->blk_id;
    int prev_id = b->prev_blk_id;
    while(prev_id!=0){
        longest_chain[prev_id] = curr_id;
        curr_id = prev_id;
        prev_id = this->blk_id_to_pointer[prev_id]->prev_blk_id;
    }
    curr_id = 1;
    // Delta is added so that jis block tk valid h wo saare txns ko account kr lenge
    // par maan lo koi block invalid h to uske txns to ignore krne h na
    // Basically pura chain hi ignore ho jayega and balances update nahi honge
    vector<long int> delta(num_peers+1,0);
    while(longest_chain.contains(curr_id)){
        curr_id = longest_chain[curr_id];
        Block* cur_block = this->blk_id_to_pointer[curr_id];

        delta[cur_block->miner] += 50;         
        for(Transaction*txn:cur_block->transactions){
            long int payer = txn->payer_id;
            long int receiver = txn->receiver_id;
            long int num_coins = txn->num_coins;
            if(this->balances[payer]<num_coins){
            // TODO: If piazza reply says to remove the remaining invalid chain, we will do it later
                return false;
            }
            delta[payer] -= num_coins;
            delta[receiver] += num_coins;
        }
    }
    for(auto i=0;i<num_peers;i++){
        this->balances[i+1]+= delta[i+1];
    }
    return true;
}

void Node:: update_tree_and_add(Block*b,Block*prev_block,bool cond){
    this->blk_id_to_pointer[b->blk_id] = b;
    if(this->longest_chain_leaf->depth < 1+prev_block->depth && this->longest_chain_leaf!=prev_block){
        if(!this->traverse_to_genesis_and_check(b)) {
            this->blk_id_to_pointer.erase(b->blk_id);
            return;
        }
    }

    this->blockchain_tree[b->prev_blk_id].insert(b->blk_id);
    b->depth = 1 + prev_block->depth;

    if(this->longest_chain_leaf->depth < b->depth){
        this->longest_chain_leaf = b;
        if(this->latest_mining_event && cond){
            events.erase(this->latest_mining_event);
            long int cancel_hone_wala_id = this->latest_mining_event->blk->blk_id;
            delete this->latest_mining_event;
            events.insert(this->generate_block_event(cancel_hone_wala_id));
        }
    }
}  

bool Node:: check_balance_validity(Event*e){
    if(e->blk->prev_blk_id == this->longest_chain_leaf->blk_id){
        vector<long int> delta(num_peers+1,0);
        delta[e->blk->miner]+=50;
        for(Transaction* txn:e->blk->transactions){
            long int payer = txn->payer_id;
            long int receiver = txn->receiver_id;
            long int num_coins = txn->num_coins;
            if(balances[payer]<num_coins){
                return false;
            }
            delta[payer] -= num_coins;
            delta[receiver] += num_coins;
        }
        for(auto i=0;i<num_peers;i++){
            this->balances[i+1]+= delta[i+1];
        }
    }
    return true;
}

void Node:: print_stats(){
    
}
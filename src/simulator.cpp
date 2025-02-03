#include "declarations.h"

int num_peers=10;
double slow_percent=20;
double low_cpu_percent=30;
double transaction_mean_time=600000;   // microseconds
double block_mean_time=600;        // seconds
long double current_time=0;     // microseconds
long double end_time = 500000;    // microseconds
long int txn_counter = 0;
long int blk_counter = 0;

unordered_map<int,unordered_map<int,int>> rhos;  // milliseconds
unordered_map<int,Node*> nodes;
set<Event*,decltype(comp)> events;
random_device rd;
mt19937 gen(rd());

void generate_network(){
    vector<int> in_tree;
    vector<int> out_tree;
    for(int i=0;i<num_peers;i++) out_tree.push_back(i+1);

    uniform_int_distribution<> dis(0, out_tree.size() - 1);
    int randomIndex = dis(gen);
    in_tree.push_back(out_tree[randomIndex]);
    swap(out_tree[randomIndex],out_tree[out_tree.size()-1]);
    out_tree.pop_back();

    while (out_tree.size()>0){
        uniform_int_distribution<> dis(0, out_tree.size() - 1);
        int randomIndex = dis(gen);
        int curr_node = out_tree[randomIndex];      // Kon sa node tree me jane wala h
        swap(out_tree[randomIndex],out_tree[out_tree.size()-1]);
        out_tree.pop_back();

        uniform_int_distribution<> dis2(0, in_tree.size() - 1);
        int randomIndex2 = dis2(gen);
        int tobe = in_tree[randomIndex2];       // Kon sa tree wala node connect hone wala h

        nodes[curr_node]->neighbours.insert(tobe);
        nodes[tobe]->neighbours.insert(curr_node);

        in_tree.push_back(curr_node);
    }

    for(int i=0;i<num_peers;i++){
        while(nodes[i+1]->neighbours.size() < 3){
            uniform_int_distribution <> dis(1,num_peers);
            int val = dis(gen);
            while(val==i+1 || nodes[i+1]->neighbours.find(val) != nodes[i+1]->neighbours.end() 
            || nodes[val]->neighbours.size()>=6){
                val = dis(gen); //wapas se chuno
            } 
            nodes[i+1]->neighbours.insert(val);
            nodes[val]->neighbours.insert(i+1);
        }
    }

    for(int i=0;i<num_peers;i++){
        for(auto j:nodes[i+1]->neighbours){
            uniform_int_distribution <> dis(10,500);
            rhos[i+1][j] = dis(gen);
            rhos[j][i+1] = dis(gen);
        }
    }
}

void generate_nodes(){
    for(int i=0;i<num_peers;i++){
        if(i<slow_percent*num_peers/100) nodes[i+1]=new Node(i+1,true,false,0);
        else nodes[i+1]=new Node(i+1,false,false,0);
    }
    vector<int> temp;
    for(int i=0;i<num_peers;i++) temp.push_back(i+1);
    shuffle(temp.begin(),temp.end(),gen);
    for(int i=0;i<low_cpu_percent*num_peers/100;i++){
        nodes[temp[i]]->is_low_cpu = true;
    }
}

void generate_events(){
    for(int i=0;i<num_peers;i++){
        events.insert(nodes[i+1]->generate_trans_event());
    }
}

void run_events(){
    while(!events.empty() && current_time<= end_time){
        Event* e = *(events.begin());
        events.erase(prev(events.end()));
        
        current_time = e->timestamp;
        e->process_event();

        if(e->event_type == "gen_trans"){
            events.insert(nodes[e->sender]->generate_trans_event());
        }
        else if(e->event_type == "gen_block"){
            events.insert(nodes[e->sender]->generate_block_event());
        }
    }
}

int main(int argc, char* argv[]) {
    generate_nodes();
    generate_network();
    generate_events();
    
    run_events();

    return 0;
} 
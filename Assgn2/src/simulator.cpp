#include "declarations.h"

int num_peers;
double slow_percent;
double malicious_node_percent;
double timeout;
long double transaction_mean_time;      // microseconds
long double block_mean_time;            // microseconds
long double current_time;               // microseconds
long double end_time;                   // microseconds
long int txn_counter = 0;
long int blk_counter = 1;
bool no_eclipse_attack = false;

unordered_map<int,unordered_map<int,int>> rhos,rhos_overlay;  // milliseconds
unordered_map<int,Node*> nodes;
set<Event*,decltype(comp)> events;
vector<long int> malicious_node_list;
random_device rd;
Malicious_Node* ringmaster;
Block* genesis_block = new Block();

// auto seed = rd();
// auto seed = 1461198227;
auto seed = 3342750228;      // seg fault
mt19937 gen(seed);

long int draw_from_uniform(long int low,long int high){
    if(low > high) {cout << "Invalid range passed in draw_from_uniform" << endl;exit(1);}
    uniform_int_distribution<long int> dis(low, high);
    return dis(gen);
}

double draw_from_exp(double lambda){
    exponential_distribution<double> dis(lambda);
    return dis(gen);
}

double draw_from_uniform_cont(double low, double high) {
    if (low > high) {
        cout << "Invalid range passed in draw_from_uniform_cont" << endl;
        exit(1);
    }
    uniform_real_distribution<double> dis(low, high);
    return dis(gen);
}


void parse_arguments(int argc, char* argv[]) {
    argparse::ArgumentParser program("Simulator");

    program.add_argument("-n","--peers")
        .default_value(20)
        .scan<'i', int>()
        .help("Number of peers in the network");

    program.add_argument("-z0","--slow_percent")
        .default_value(80.0)
        .scan<'g', double>()
        .help("Percentage of slow nodes");

    program.add_argument("-Tt","--timeout")
        .default_value(20.0)
        .scan<'g', double>()
        .help("Timeout value for get requests");

    program.add_argument("-mnp","--malicious_node_percent")
        .default_value(20.0)
        .scan<'g', double>()
        .help("Malicious node percentage");

    program.add_argument("-Ttx","--txn_interarrival")
        .default_value(5000000.0L)
        .scan<'g', long double>()
        .help("Mean transaction generation time (microseconds)");

    program.add_argument("-I","--block_interarrival_time")
        .default_value(600000000.0L)
        .scan<'g', long double>()
        .help("Mean block generation time (microseconds)");

    program.add_argument("-e","--end_time")
        .default_value(12000000000.0L)
        .scan<'g', long double>()
        .help("Simulation end time (microseconds)");

    program.add_argument("-nec", "--no-eclipse-attack")
        .default_value(false)
        .implicit_value(true)
        .help("Disable eclipse attack");

    try {
        program.parse_args(argc, argv);

        num_peers = program.get<int>("--peers");
        slow_percent = program.get<double>("--slow_percent");
        malicious_node_percent = program.get<double>("--malicious_node_percent");
        timeout = program.get<double>("--timeout");
        transaction_mean_time = program.get<long double>("--txn_interarrival");
        block_mean_time = program.get<long double>("--block_interarrival_time");
        end_time = program.get<long double>("--end_time");
        no_eclipse_attack = program.get<bool>("--no-eclipse-attack");

    } catch (const exception& e) {
        cerr << "Argument parsing error: " << e.what() << endl;
        cerr << program;
        exit(1);
    }
}

// Implements the network topology.
// First of all the spanning tree is generated randomly.
// This ensures that the graph is connected.
// Then the requirement that the degree of each node is between 3 and 6 
// (bounds included) is satisfied individually for each node.
void generate_graph(bool overlay=false){
    vector<int>node_list;
    for(int i=0;i<num_peers;i++){
        if(overlay){
            if(nodes[i+1]->is_malicious) node_list.push_back(i+1);
        }
        else node_list.push_back(i+1);
    }
    vector<int> in_tree;
    vector<int> out_tree = node_list;

    int randomIndex = draw_from_uniform(0,out_tree.size()-1);
    in_tree.push_back(out_tree[randomIndex]);
    swap(out_tree[randomIndex],out_tree[out_tree.size()-1]);
    out_tree.pop_back();

    while (out_tree.size()>0){
        randomIndex = draw_from_uniform(0,out_tree.size()-1);
        int curr_node = out_tree[randomIndex];      // Kon sa node tree me jane wala h
        swap(out_tree[randomIndex],out_tree.back());
        out_tree.pop_back();
        
        randomIndex = draw_from_uniform(0,in_tree.size()-1);
        int tobe = in_tree[randomIndex];       // Kon sa tree wala node connect hone wala h
        
        unordered_set<int>* neigh = get_neighbours(nodes[tobe],overlay);
        
        while(neigh->size()>=6){
            randomIndex = draw_from_uniform(0,in_tree.size()-1);
            tobe = in_tree[randomIndex];
            neigh = get_neighbours(nodes[tobe],overlay);
        }
        
        unordered_set<int>* other = get_neighbours(nodes[curr_node],overlay); 
        other->insert(tobe);
        neigh->insert(curr_node);
        in_tree.push_back(curr_node);
    }
    
    for(int i:node_list){
        unordered_set<int>* neigh = get_neighbours(nodes[i],overlay);
        for(auto j:*neigh){
            if(overlay)rhos_overlay[i][j] = rhos_overlay[j][i] = draw_from_uniform_cont(1,10);
            else rhos[i][j] = rhos[j][i] = draw_from_uniform_cont(10,500);
        }
    }
    if(node_list.size() <= 3){cerr << "Too less nodes" << endl;exit(1);}

    for(int i:node_list){
        unordered_set<int>* neigh = get_neighbours(nodes[i],overlay);
        
        while(neigh->size() < 3){
            int val = node_list[draw_from_uniform(0,node_list.size()-1)];
            unordered_set<int>* other = get_neighbours(nodes[val],overlay);
            
            while(val==i || neigh->find(val) != neigh->end() || other->size()>=6){
                val = node_list[draw_from_uniform(0,node_list.size()-1)];
                other = get_neighbours(nodes[val],overlay);
                // wapas se chuno
            } 
            neigh->insert(val);
            other->insert(i);
        }
    }
}

// The nodes are initialised with the mentioned parameters randomly. 
void generate_nodes(){
    long int malicious_nodes = malicious_node_percent*num_peers/100;
    long int slow_nodes = slow_percent*num_peers/100;

    for(int i=0;i<num_peers;i++){
        nodes[i+1]=new Node(i+1,false,false);
    }

    vector<int> temp;
    for(int i=0;i<num_peers;i++) temp.push_back(i+1);
    shuffle(temp.begin(),temp.end(),gen);

    for(int i=0;i<malicious_nodes;i++){
        delete nodes[temp[i]];
        nodes[temp[i]] = new Malicious_Node(temp[i],false,true);
        malicious_node_list.push_back(temp[i]);
    }

    shuffle(temp.begin(),temp.end(),gen);

    if(malicious_nodes + slow_nodes > num_peers) {cout << "Impossible" << endl;exit(1);}

    long int c=0;
    while(slow_nodes > 0){
        if(!nodes[temp[c]]->is_malicious){
            nodes[temp[c]]->is_slow = true;
            slow_nodes--;
        }
        c++;
    }

    long int mal_index = draw_from_uniform(0,malicious_node_list.size()-1);
    ringmaster = (Malicious_Node*)nodes[malicious_node_list[mal_index]];
    ringmaster->hash_power = double(malicious_node_list.size())/num_peers;
}

// Generates starting events for the simulation.
void generate_events(bool block){
    for(int i=0;i<num_peers;i++){
        if(block && (!nodes[i+1]->is_malicious || nodes[i+1]==ringmaster)) events.insert(nodes[i+1]->generate_block_event());
        else events.insert(nodes[i+1]->generate_trans_event());
    }
}

// Invoke the events with the earliest event coming first.
void run_events(){
    while(!events.empty() && current_time<= end_time){
        auto it = events.begin();
        Event* e = *it;
        current_time = e->timestamp;
        
        e->process_event();
        
        if(e->event_type == "gen_trans"){
            events.insert(nodes[e->sender]->generate_trans_event());
        }
        else if(e->event_type == "gen_block"){
            events.insert(nodes[e->sender]->generate_block_event());
        }
        
        events.erase(it);
        delete e;
    }

    while(!events.empty()){
        auto it = events.begin();
        Event* e = *it;
        current_time = e->timestamp;
        
        if(e->event_type!="gen_block" && e->event_type!="gen_trans") e->process_event();        
        
        events.erase(it);
        delete e;   
    }
}

int main(int argc, char* argv[]) {
    cout << seed << endl;
    genesis_block->blk_id = 1;
    genesis_block->prev_blk_id = 0;
    genesis_block->timestamp = 0;
    genesis_block->depth = 1;
    parse_arguments(argc,argv);
    generate_nodes();
    generate_graph(false);
    generate_graph(true);

    ofstream outFile("outputs/peer_network_edgelist.txt",ios::app);
    outFile << ringmaster->node_id << endl;
    for(int i:malicious_node_list){
        if(nodes[i]==ringmaster) continue;
        outFile << i << endl;
    }

    for(int i=0;i<num_peers;i++){
        for(int j:*(get_neighbours(nodes[i+1],false))){
            outFile << i+1 << " " << j << endl;
        }
    }

    outFile.close();
    outFile = ofstream("outputs/overlay_network_edgelist.txt",ios::app);
    outFile << ringmaster->node_id << endl;
    for(int i:malicious_node_list){
        if(nodes[i]==ringmaster) continue;
        outFile << i << endl;
    }
    for(int i:malicious_node_list){
        for(int j:*(get_neighbours(nodes[i],true))){
            outFile << i << " " << j << endl;
        }
    }
    outFile.close();

    generate_events(false);
    generate_events(true);
    run_events();
    
    ofstream outFile2("outputs/peer_stats.csv",ios::out);
    
    outFile2 << "Node_id,Chain_Blocks,Total_Blocks,is_slow,hash_power,Length of longest chain" << endl;
    for(int i = 0;i<num_peers;i++){
        nodes[i+1]->print_tree_to_file();
        nodes[i+1]->print_stats(outFile2);
        nodes[i+1]->outFile.close();
    }
    return 0;
} 
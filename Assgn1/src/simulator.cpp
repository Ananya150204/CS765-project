#include "declarations.h"

int num_peers;
double slow_percent;
double low_cpu_percent;
long double transaction_mean_time;      // microseconds
long double block_mean_time;            // microseconds
long double current_time;               // microseconds
long double end_time;                   // microseconds
long int txn_counter = 0;
long int blk_counter = 1;

unordered_map<int,unordered_map<int,int>> rhos;  // milliseconds
unordered_map<int,Node*> nodes;
set<Event*,decltype(comp)> events;
random_device rd;
mt19937 gen(rd());

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

    program.add_argument("-z1","--low_cpu_percent")
        .default_value(20.0)
        .scan<'g', double>()
        .help("Percentage of nodes with low CPU power");

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

    try {
        program.parse_args(argc, argv);

        num_peers = program.get<int>("--peers");
        slow_percent = program.get<double>("--slow_percent");
        low_cpu_percent = program.get<double>("--low_cpu_percent");
        transaction_mean_time = program.get<long double>("--txn_interarrival");
        block_mean_time = program.get<long double>("--block_interarrival_time");
        end_time = program.get<long double>("--end_time");

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
        while(nodes[tobe]->neighbours.size()>=6){
            randomIndex2 = dis2(gen);
            tobe = in_tree[randomIndex2];
        }

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


// The nodes are initialised with the mentioned parameters randomly. 
void generate_nodes(){
    long int low_cpu_nodes = low_cpu_percent*num_peers/100;
    long int high_cpu_nodes = num_peers - low_cpu_nodes;
    long double h = double(1)/(10*high_cpu_nodes+low_cpu_nodes);

    for(int i=0;i<num_peers;i++){
        if(i<slow_percent*num_peers/100) nodes[i+1]=new Node(i+1,true,false,10*h);
        else nodes[i+1]=new Node(i+1,false,false,10*h);
    }
    vector<int> temp;
    for(int i=0;i<num_peers;i++) temp.push_back(i+1);
    shuffle(temp.begin(),temp.end(),gen);

    for(int i=0;i<low_cpu_percent*num_peers/100;i++){
        nodes[temp[i]]->is_low_cpu = true;
        nodes[temp[i]]->hash_power /= 10;
    }
}

// Generates starting events for the simulation.
void generate_events(bool block){
    for(int i=0;i<num_peers;i++){
        if(block) events.insert(nodes[i+1]->generate_block_event());
        else events.insert(nodes[i+1]->generate_trans_event());
    }
}

// Invoke the events with the earliest event coming first.
void run_events(){
    while(!events.empty() && current_time<= end_time){
        Event* e = *(events.begin());
        events.erase(events.begin());
        
        current_time = e->timestamp;
        e->process_event();

        if(e->event_type == "gen_trans"){
            events.insert(nodes[e->sender]->generate_trans_event());
        }
        else if(e->event_type == "gen_block"){
            events.insert(nodes[e->sender]->generate_block_event());
        }
        delete e;
    }
}

int main(int argc, char* argv[]) {
    parse_arguments(argc,argv);
    generate_nodes();
    generate_network();

    ofstream outFile("outputs/peer_network_edgelist.txt",ios::app);
    for(int i=0;i<num_peers;i++){
        for(int j:nodes[i+1]->neighbours){
            outFile << i+1 << " " << j << endl;
        }
    }
    outFile.close();

    generate_events(false);
    generate_events(true);
    run_events();
    
    ofstream outFile2("outputs/peer_stats.csv",ios::out);
    
    outFile2 << "Node_id,Chain_Blocks,Total_Blocks,is_high_CPU,is_slow,hash_power" << endl;
    for(int i = 0;i<num_peers;i++){
        nodes[i+1]->print_tree_to_file();
        nodes[i+1]->print_stats(outFile2);
        nodes[i+1]->outFile.close();
    }

    return 0;
} 
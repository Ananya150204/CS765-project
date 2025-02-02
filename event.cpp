#include <bits/stdc++.h>
#ifndef c
#include "transaction.cpp"
#endif 
using namespace std;
typedef long double ld;
// Gen_trans, for_trans, rec_trans

auto comp = [](Event*&a,Event*&b)->bool{
    return a->timestamp > b->timestamp;
};

class Event{
    public:
        string event_type;
        ld timestamp;
        Transaction* txn;
        
        Event(string event_type,ld timestamp,Transaction*txn){
            this->event_type = event_type;
            this->timestamp = timestamp;
            this->txn = txn;
        }
};
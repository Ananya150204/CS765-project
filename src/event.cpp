#include "declarations.h"

Event::Event(string event_type,ld timestamp,Transaction*txn){
    this->event_type = event_type;
    this->timestamp = timestamp;
    this->txn = txn;
}
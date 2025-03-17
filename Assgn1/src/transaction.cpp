#include "declarations.h"

Transaction::Transaction(long int txn_id,long int payer_id,long int receiver_id,long int num_coins){
    this->txn_id = txn_id;
    this->payer_id = payer_id;
    this->receiver_id = receiver_id;
    this->num_coins = num_coins;
}
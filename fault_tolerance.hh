#pragma once

#include "server_bck.hh"
#include <iterator>
#include "process_operation.hh"
#include "process_req_bck.hh"
#include <sstream>
#include "utils.h"
#include "replication.h"

extern string sep;
extern int sep_len;
 
// Storing the current state of key value store in memory
void checkpoint();

// Recovering the stored checkpoint
void recover_from_checkpoint(ifstream& recover);

// Storing the incoming requests per user in the memory
void log_request(const request& req, const string& file);

// Recovering from the stored logs
void recover_from_log(const string& file);

// Recovering the state from other backend servers
bool updates_from_backends();
#pragma once


#include <iostream>
#include <unordered_map>
#include <time.h>
#include <stdlib.h>
#include <unordered_set>
#include <iterator>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include "protobuf.h"
#include "utils.h"
#include <sys/stat.h> 
//#include <resolv.h>

using namespace std;
using namespace google::protobuf::io;

unordered_map <char, vector<string>> server_assignment;
vector <string> backend_servers;


string ip_addr; // IP Address of master server
int port; // Port Number of master server

int fd_range = 0;
string smtp_server = "127.0.0.1:2500";

// Assign replicas randomly to each alphabet
void assign_servers();

// Function for debugging
void print_things();

// Checkpointing the alphabet-replicas assignment
void master_checkpoint();

// Recover master from the checkpoint
void recover_master(ifstream& input);
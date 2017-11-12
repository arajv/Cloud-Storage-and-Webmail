/*
 * serverbck.h
 *
 *  Created on: Apr 10, 2017
 *      Author: cis505
 */

#ifndef _SERVERBCKH_
#define _SERVERBCKH_

#include <unordered_map>
#include <map>
#include <string>
#include <iostream>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protobuf.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unordered_set>
#include "request.pb.h"

using namespace std;
using namespace google::protobuf::io;

/*table for storing data
 * row is user name
 * col is type:path where type is mail,file,pass
 * cell is the actual content
 */
extern unordered_map<string,map<string,vector<string>>> key_val_store;

// Binary Locks per user (row of key value store) 
extern unordered_map<string,pthread_mutex_t> locks;

// Read Write Lock on the complete key value store
// (Exclusive Lock is taken during checkpointing and shared lock is taken during requests)
extern pthread_rwlock_t masterlock;

struct server_addr{
	string ip;
	int port;
};

// Addresses of other backend servers
extern vector <server_addr> other_servers;

// Address of master backend server
extern server_addr mstr_srv_addr;

// Own Address
extern server_addr my_addr;


extern bool verbose;
extern int listen_fd;

// Index of the server in the text file contanining addresses
extern int server_num;

void listen_for_connections();
void accept_new_connections();

// Function executed by a Thread for processing incoming requests 
void * worker(void *arg);

// Launch Thread for checkpointing at regular intervals
void launch_checkpointing_thread();

// Function executed by the checkpointing thread 
void* chkpnt_worker(void *arg);

// Recovering from the logs stored in the memory
void log_recovery(bool temp_recovery);

// To create folders for storing the checkpoint and log files
void make_necessary_folders();

// Debugging functions
void print_map();
void test_element();


google::protobuf::uint32 readHdr(char *buf);
request readBody(int comm_fd,google::protobuf::uint32 siz);
void send_response(int fd,request res_payload);

#endif

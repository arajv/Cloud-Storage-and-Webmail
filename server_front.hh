#ifndef _SERVERFRNTH_
#define _SERVERFRNTH_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "request.pb.h"
#include "smtp_client.h"
#include "utils.h"

using namespace std;
using namespace google::protobuf::io;

struct server_addr{
        string ip;
        int port;
};

struct request_details{
	string req_type;
	int req_num;
	bool success;
	string row;
	string col;
	vector<string> val1;
	vector<string> val2;
};

extern hash<string> str_hash;
extern int listen_fd;
extern bool verbose;
extern long long num_requests;
extern server_addr mstr_srv_addr,my_addr;
extern server_addr master_back;
extern unordered_map<string,string> sids;
extern unordered_map<string,vector<string>> clients_cache;

request_details readBody(int comm_fd,google::protobuf::uint32 siz);

//listen for incoming connections
void listen_for_connections();

//accept new connections
void accept_new_connections();

/*Handle incoming requests
 * and process accordingly
 */
void * worker(void *arg);

/*Render page based on static html file
 * and dynamic content from backend
 * Also perform necessary operations
 */
void render_page(int fd,string page_name,string request,string content,string cookies[2]);

/*Render page from static
 * html file
 */
void render_simple_page(int& fd, string file_name);
request_details send_and_receive(request req_payload);

/*Clean text by replacing ascii values
 * with the character values
 */
void clean_text(string& text);

#endif

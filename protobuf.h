#pragma once

#include "request.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

using namespace std;
using namespace google::protobuf::io;


// Receiving msg on a given file descriptor 
int readmsg(int fd, request& payload);

// Send message on a given file descriptor
int sendmsg(int fd, const request& payload);
google::protobuf::uint32 readHdr(char *buf);

// Send and Receive a reply from a given ip and port
bool send_recv(request& req_recv, const request& req_send, string ip, int port);

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
//#include <pthread.h>
#include <new>
#include <vector>
#include <cctype>
#include <iostream>
#include <signal.h>
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include <dirent.h>
#include <ctime>
#include <fstream>
#include <resolv.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <sys/types.h>
#include <netdb.h>
//#include "protobuf.h"

using namespace std;

//int sockfd;



bool do_write(int fd, const char*msg, int len);

// Finding the ip address of smtp servers of other domains
char* parse_record (unsigned char *buffer, size_t r, const char *section, ns_sect s, int idx, ns_msg *m);

// Reading a reply from a given fd
void read_reply(int sockfd, string& reply);

// Send mail to a SMTP server in a specific format
int sendmail(int sockfd, const string& sender, const string& receiver, const string& subject, const string& message);

// Sending mail to all the receivers (This function in turn calls the above function)
bool smtpclient(const string& sender, vector<string>& receivers, const string& subject, const string& message);

#pragma once

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h>

using namespace std;

// Seperating ip and port from an address
bool get_ip_port (string arg, string& ip_addr, int& port);

// Close all the given fds
void close_fds(const vector<int>& fds);

extern const int R; // Read Quorum
extern const int W; // Write Quorum
extern int num_replications; // Number of replicas
extern int deadlock_wait; // seconds 
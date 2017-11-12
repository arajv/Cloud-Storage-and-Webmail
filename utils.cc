#include "utils.h"

const int R = 2; // Read Quorum
const int W = 2; // Write Quorum
int num_replications = 3; // Number of times a specific data is stored
int deadlock_wait = 3; // seconds

bool get_ip_port (string arg, string& ip_addr, int& port){
	size_t found = arg.find(":");
    if (found==string::npos){
    	return false;
    }
    else{
	    ip_addr = arg.substr(0,found);
		port = stoi(arg.substr(found+1,arg.length()-found-1));
		return true;
	}
}

void close_fds(const vector<int>& fds){
	for (unsigned int i=0; i<fds.size(); i++){
		close(fds[i]);
	}
}

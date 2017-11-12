#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <unordered_map>
#include <unistd.h>
#include <time.h>
#include <tuple>
#include <algorithm>

using namespace std;

int t_num = 0;
bool v = false;
int all_threads[100];
int count =0;
vector<tuple<string, int>> frontServers;

typedef pair<string, int> myPairType;

bool mycompare(tuple<string, int > &left, tuple<string, int > &right)
{
	return get<1>(left) < get<1>(right);
}

struct compareSecond
{
	bool operator()(const myPairType& left, const myPairType& right) const
	{
		return left.second , right.second;
	}

};

string getMin(unordered_map<string, int> myMap)
{
	pair<string , int> min  = *min_element(myMap.begin(), myMap.end(), compareSecond());
	return min.first;
}


int sendData(int sckt, char *pdata, int datalen)
{
    //unsigned char *pdata = (unsigned char *) data;
    int numSent;

    // send() can send fewer bytes than requested,
    // so call it in a loop until the specified data
    // has been sent in full...

    while (datalen > 0) {
      numSent = send(sckt, pdata, datalen, 0);
      if (numSent == -1) return -1;
      pdata += numSent;
      datalen -= numSent;
    }

    return 0;
}

// send to handle when server not running
int sendDataNoReply(int sckt, char *pdata, int datalen)
{
    //unsigned char *pdata = (unsigned char *) data;
    int numSent;

    // send() can send fewer bytes than requested,
    // so call it in a loop until the specified data
    // has been sent in full...

    while (datalen > 0) 
    {
      numSent = send(sckt, pdata, datalen, MSG_NOSIGNAL);
      if (numSent == -1) return -1;
      pdata += numSent;
      datalen -= numSent;
    }

    return 0;
}

void * worker(void *arg) // thread for each requst from client
{
	int comm_fd = *(int*)arg;
	char buf[1000];
	string strBuf = "";
	string tmp="";
	int ret,loc;
	string first_request = "";
	int get_flag = 0 ;
	int count = 0;
	//count++;
	//cout<<"thread number  "<<count<<endl;
	//cout<<"Number of servers on file "<<frontServers.size()<<endl;

	while(true)
	{
		//cout<<"into while"<<endl;
		count++;
		/*if(count == 5)
			exit(1);*/
		ret = read(comm_fd, &buf, 1000);
		tmp = string(&buf[0],ret);
		strBuf.append(tmp);
		//cout<<strBuf<<endl;
		memset(buf,0,1000);


		while((loc = strBuf.find("\r\n\r\n"))!=string::npos)
		{
			// get first line from header
			int firstLineENd = strBuf.find("\r\n");
			string firstLine = strBuf.substr(0,firstLineENd);

			// get request type from first line
			int firstSpace = firstLine.find(" ");
			string requestType = firstLine.substr(0,firstSpace);	

			// find function name
			string rest_first_line = firstLine.substr(firstSpace+1);
			int space_index = rest_first_line.find(" ");
			string function_name = rest_first_line.substr(0, space_index);

			if(requestType == "GET")
			{
				if(function_name == "/")
				{// accept connection from client, find load from front end server and forward the client to a selected front end server
					//cout<<"get request"<<endl;
					//cout<<strBuf<<endl;
					//cout<<"frontend server size"<<frontServers.size()<<endl;
					
					// ping all addresses
					for(int i = 0; i<frontServers.size(); i++)
					{
						// saving ip and port 
						string ipAndPort = get<0>(frontServers[i]);
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);
						// setting up sockets
						int frontSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(frontSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in frontServaddr;
						bzero(&frontServaddr,sizeof(frontServaddr));
						frontServaddr.sin_family = AF_INET;
						frontServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(frontServaddr.sin_addr));

						connect(frontSockfd, (struct sockaddr*)&frontServaddr, sizeof(frontServaddr));

						// seding a LOAD request
						stringstream wsss;
						wsss << "LOAD\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"count in get "<<count<<endl;
    					sendDataNoReply(frontSockfd, (char*)headers.c_str(), headers.size());
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";
    					int fromServer = read(frontSockfd, &temp, 1000);
    					//cout<<"#return"<<"  "<<fromServer<<endl;
    					int load;
    					if(fromServer == -1)
    						load = 1000000;
    					else
    					{
							tmp = string(&temp[0],fromServer);    						
    						// extract number from buffer
    						int end = tmp.find("\r\n");
    						load = stoi(tmp.substr(0,end));
    					}
    					//cout<<"load is "<<load<<endl;
    					// store in vector
    					get<1>(frontServers[i])=load;
    					fromServer = 0;
    					// close
    					close(frontSockfd);

					}

					sort(frontServers.begin(), frontServers.end(), mycompare);
					// server address to forward to

					string minAddress = get<0>(frontServers[0]);

					//cout<<minAddress<<endl;

					stringstream wsss;
					wsss << "HTTP/1.1 302 Found\r\n" 
     				//<< "Connection: Close\r\n"
     				<< "Location: http://"<<minAddress<<"\r\n"
     				<< "Content-Length: 0"<< "\r\n"
     				<< "\r\n";

    				string headers  = wsss.str();
    				sendData(comm_fd, (char*)headers.c_str(), headers.size()); // forwarding to a selected server
    				//close(comm_fd);

					strBuf = strBuf.substr(loc+4);
					//cout<<strBuf<<endl;
				}
				else if(function_name == "/favicon.ico")
				{ // handle favicon 
					//cout<<"other get request"<<endl<<endl;
					stringstream wsss;
					wsss << "HTTP/1.1 204 \r\n" 
     	 			<< "\r\n";

     	 			string headers  = wsss.str();
    				sendData(comm_fd, (char*)headers.c_str(), headers.size());
					//cout<<strBuf<<endl;
					strBuf = strBuf.substr(loc+4);
				}
				else
				{// handles the case when a front end server fails and redirects the request to a working front end server
					//cout<<"get user request"<<endl;
					//cout<<strBuf<<endl;
					
					// ping all addresses
					//cout<<"frontend server size"<<frontServers.size()<<endl;
					for(int i = 0; i<frontServers.size(); i++)
					{
						// saving ip and port 
						string ipAndPort = get<0>(frontServers[i]);
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);
						//cout<<ip<<endl;
						// setting up sockets
						int frontSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(frontSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in frontServaddr;
						bzero(&frontServaddr,sizeof(frontServaddr));
						frontServaddr.sin_family = AF_INET;
						frontServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(frontServaddr.sin_addr));

						connect(frontSockfd, (struct sockaddr*)&frontServaddr, sizeof(frontServaddr));

						// seding a LOAD request
						stringstream wsss;
						wsss << "LOAD\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"count in get "<<count<<endl;
    					sendDataNoReply(frontSockfd, (char*)headers.c_str(), headers.size());
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";
    					int fromServer = read(frontSockfd, &temp, 1000);
    					//cout<<"#read"<<"  "<<fromServer<<endl;
    					int load;
    					if(fromServer == -1)
    						load = 1000000;
    					else
    					{
							tmp = string(&temp[0],fromServer);    						
    						// extract number from buffer
    						int end = tmp.find("\r\n");
    						load = stoi(tmp.substr(0,end));
    					}
    					//cout<<"load is "<<load<<endl;
    					// store in vector
    					get<1>(frontServers[i])=load;
    					close(frontSockfd);

					}
					//cout<<"out of for loop"<<endl;

					sort(frontServers.begin(), frontServers.end(), mycompare);
					// server address to forward to

					string minAddress = get<0>(frontServers[0]);

					//cout<<minAddress<<endl;

					stringstream wsss;
					wsss << "HTTP/1.1 302 Found\r\n" 
     				//<< "Connection: Close\r\n"
     				<< "Location: http://"<<minAddress<<function_name<<"\r\n"
     				<< "Content-Length: 0"<< "\r\n"
     				<< "\r\n";

    				string headers  = wsss.str();
    				//cout<<headers<<endl;
    				sendData(comm_fd, (char*)headers.c_str(), headers.size());
    				//close(comm_fd);

					strBuf = strBuf.substr(loc+4);

				}

				
			}
			else
			{
				cout<<"other request"<<endl;
				strBuf = strBuf.substr(loc+4);
				//exit(1);
			}


		}


	}

}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		fprintf(stderr, "Please enter a file name and the index for master\n");
		exit(1);
	}
	string fileName = argv[1];

	ifstream ifs(fileName);
	if(!ifs.is_open())
	{
		fprintf(stderr, "unable to open file\n");
		exit(1);
	}

	string line;
	string masterAddr = "";
	bool skip = 1;
	while(getline(ifs, line))// storing front end server detaisl from a text file
	{
		//frontServers[line] = 0;
		if(skip == 0)
			frontServers.push_back(make_tuple(line, 0));
		if(skip == 1)
		{
			masterAddr = line;
		}
		skip = 0;
	}

	string ipAndPort = masterAddr;
	char *ipAndPortChar = &ipAndPort[0u];
	char * ip = strtok(ipAndPortChar, ":");
	char * frontstringPort = strtok(NULL, ":");
	int frontPort = stoi(frontstringPort, NULL, 10);
	//cout<<masterAddr<<endl;
	// setting up sockets
	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		fprintf(stderr, "cannot open socket\n");
	}
	struct sockaddr_in frontServaddr;
	bzero(&frontServaddr,sizeof(frontServaddr));
	frontServaddr.sin_family = AF_INET;
	frontServaddr.sin_port = htons(frontPort);
	inet_pton(AF_INET, ip, &(frontServaddr.sin_addr));

	int ret;
	int enable = 1;

	ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	if(ret == -1){
		cerr<<"setsockopt(SO_REUSEADDR) failed"<<endl;
	}

	bind(listen_fd,(struct sockaddr*)&frontServaddr, sizeof(frontServaddr));

	listen(listen_fd, 100);
	while(true){// handle request from each client by spawning a thread
		
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int *fd = (int*)malloc(sizeof(int));
		*fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
		all_threads[t_num] = *fd;//change to vectorfs
		t_num++;

		if(v==true)
		{	
			fprintf(stderr, "New connection\n");	
			//cout<<"new connection after counting : "<<count<<endl;
		}
		//cout<<fd<<"  file desriptor value"<<endl;
		pthread_t thread;
		//pthread_create(&thread, NULL, worker, fd);
		pthread_create(&thread, NULL, worker, fd);
	}
	return 0;
}
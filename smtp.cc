#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <pthread.h>
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
#include "protobuf.h"
#include <fstream>
#include "replication.h"



int debug = 0; // -v (Printing debug output)
int counter = 5;
unordered_set<int> fds; // For storing file descriptors of all open connections
vector <pthread_t> thrds;
string master_ip;
int master_port;

// My address
int port = 2500;
string ip_addr = "127.0.0.1";


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void erase_from_set(int);
void add_to_set(int);
bool do_write(int, const char*, int);
void* worker(void* arg);
void ctrlc_handler(int sig);


int main(int argc, char *argv[])
{	
	// Registering the signal handler
	signal(SIGINT, ctrlc_handler);

	int c;
	char* filename;

	if (strcmp(argv[optind],"-v") !=0){
		filename = argv[optind];
	}

	// Reading command line inputs
	while ((c = getopt(argc, argv, "v")) != -1){
		switch (c){
			case 'v':
				debug = 1;
				if (optind < argc){
					filename = argv[optind];
				}
				break;
			case '?':
				cout << "Unknown parameter" << endl;
				break;
			default:
				exit(0);
				break;
		}

	}

	ifstream servers(filename);
	string line;
	if (servers.is_open()){
		int i = 0;
		while(getline(servers,line)){			
			if (i==0){
				
				get_ip_port (line, master_ip, master_port);
				break;				
				
			}
			// else if (i==1){
				
			// 	get_ip_port (line, ip_addr, port);
			// 	break;
			// }
			i++;
		}
		servers.close();
	}

	// Creating Socket

	int sockfd = socket(PF_INET,SOCK_STREAM,0);
	int enable = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(port);
	inet_pton(PF_INET, ip_addr.c_str(), &(server_addr.sin_addr));

	int b = bind(sockfd, (sockaddr*)&server_addr, sizeof(server_addr));
	cout << port << endl;
	listen(sockfd, 5);
	
	// Accepting Connections from clients and assigning them to different threads
	while(true){
		sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		int* clientfd = new int;
		*clientfd = accept(sockfd, (sockaddr*)&client_addr, &addrlen);
		add_to_set(*clientfd);
		char greet[] = "220 localhost OK Service ready\r\n";		
		bool w = do_write(*clientfd, greet, strlen(greet));
		pthread_t thread;
		pthread_create(&thread,NULL,worker,clientfd);		
		thrds.push_back(thread);
	}

  return 0;
}

// Synchronous delete from unordered set
void erase_from_set(int fd){
	pthread_mutex_lock(&mutex);
	fds.erase(fd);
	pthread_mutex_unlock(&mutex);
}

// Synchronous insert into unordered set
void add_to_set(int fd){
	pthread_mutex_lock(&mutex);
	fds.insert(fd);
	pthread_mutex_unlock(&mutex);	
}

bool do_write(int fd, const char*msg, int len){
	int index = 0;
	while (index<len){
		int n = write(fd, &msg[index], len-index);
		if (n<0){
			return false;
		}
		index+=n;
	}
	return true;
}

// Handling Ctrl+C command
void ctrlc_handler(int sig){
	pthread_mutex_lock(&mutex);
	for (unordered_set<int>::iterator it= fds.begin(); it!=fds.end(); it++){		
		int clientfd = *it;
		char shut[] = "421 Server shutting down\r\n";
		do_write(clientfd, shut, strlen(shut));		
		close(clientfd);		
	}
	pthread_mutex_unlock(&mutex);
	if (debug==1){		
		fprintf(stderr, "%s\n", "421 Server shutting down");						
	}
	for (unsigned int i=0; i<thrds.size(); i++){
		pthread_kill(thrds[i],SIGUSR1);		
		pthread_join(thrds[i],NULL);		
	}
	exit(0);
}

void sig_handler(int sig){
	pthread_exit(NULL);
}

bool Isspace (char i) { return (i==' '); }


void* worker(void* arg){
	// Registering the user defined signal
	signal(SIGUSR1, sig_handler);

	int clientfd = *(int*)arg;
	char* msg = new char[1000000];
	char* temp;
	int index = 0;
	int status = 0;
	int data = 0;

	char err500[] = "500 Syntax Error, command unrecognized";
	char err501[] = "501 Syntax Error in parameters or arguments";
	char err503[] = "503 Bad Sequence of Commands";
	char err550[] = "550 Requested action not taken: mailbox unavailable";

	char response221[] = "221 Service closing transmission channel";

	char response[] = "250 OK";
	char response354[] = "354 OK";
	char crlf[] = "\r\n";


	vector<string> recipient;
	string sender;
	//vector<string> unknowns;


	if (debug==1){
		fprintf(stderr, "[%d] %s\n", clientfd, "New connection");
	}
	while(true){
		
		//cout << "place1" << endl;
		int n = read(clientfd, &msg[index], 1); // Reading character by character
		//cout << "place2" << endl;
		//msg[index+1]='\0';
		//cout << msg << endl;
		if (n<0){
			close(clientfd);
			erase_from_set(clientfd);			
			pthread_exit(NULL);
		}
		
		else if (n>0){
			if (msg[index]=='\n' && index>4){

				//string msg_str(msg);
				
				if (debug==1){
					fprintf(stderr, "[%d] %s", clientfd, "C: ");
					msg[index-1] = '\0';
					fprintf(stderr, "%s\n", msg);
					msg[index-1] = '\r';
				}

				// if first 4 characters are "helo"
				if ((char)tolower(msg[0])=='h' && (char)tolower(msg[1])=='e' && (char)tolower(msg[2])=='l' && (char)tolower(msg[3])=='o'){
					 
					if(msg[4]==' ' && index>6){ // only if echo is followed by a space or \r\n
						
						if (status==0 || status==1){
							char prefix[] = "250 localhost\r\n";
							do_write(clientfd, prefix, strlen(prefix));		
							
							status = 1;
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s", "250 localhost");
								msg[index-1] = '\0';
								fprintf(stderr, "%s\n", &msg[4]);							
							}
						}

						else if (status==2 || status==3){							
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}	
						}
						
					}

					else if (msg[4]!= ' ' && index>=6){
						
						do_write(clientfd, err500, strlen(err500));
						do_write(clientfd, crlf, 2);
						if (debug==1){
							fprintf(stderr, "[%d] %s", clientfd, "S: ");
							fprintf(stderr, "%s\n", err500);							
						}
						
					}

					else if ((msg[4]!= ' ' && index==5) || (msg[4]==' ' && index==6)){						
						do_write(clientfd, err501, strlen(err501));
						do_write(clientfd, crlf, 2);
						if (debug==1){
							fprintf(stderr, "[%d] %s", clientfd, "S: ");
							fprintf(stderr, "%s\n", err501);							
						}
						
					}

					delete[] msg;
					msg = new char[1000000];
					index = 0;
				}				

				// MAIL FROM:
				else if (index>10 && (char)tolower(msg[0])=='m' && (char)tolower(msg[1])=='a' && (char)tolower(msg[2])=='i' && 
						(char)tolower(msg[3])=='l' && (char)tolower(msg[4])==' ' && (char)tolower(msg[5])=='f' && (char)tolower(msg[6])=='r' && 
						(char)tolower(msg[7])=='o' && (char)tolower(msg[8])=='m' && (char)tolower(msg[9])==':'){					
					if (index >=12){
						if (status==1){
							msg[index-1] = '\0';
							string from_add(&msg[10]);
							msg[index-1] = '\r';
							from_add.erase(remove_if(from_add.begin(), from_add.end(), Isspace), from_add.end());							
							size_t found = from_add.find('@');
							if (found!= string::npos){
								do_write(clientfd, response, strlen(response));
								do_write(clientfd, crlf, 2);
								if (debug==1){
									fprintf(stderr, "[%d] %s", clientfd, "S: ");
									fprintf(stderr, "%s\n", response);							
								}
								sender = from_add;
								status = 2;
							}
						}
						else{
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}	
						}
					}
					else{
						if (status==1){
							do_write(clientfd, err501, strlen(err501));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err501);							
							}
						}
						else{
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}
						}
					}
					
					delete[] msg;
					msg = new char[1000000];
					index = 0;	
				}

				// RCPT TO:
				else if (index>8 && (char)tolower(msg[0])=='r' && (char)tolower(msg[1])=='c' && (char)tolower(msg[2])=='p' && 
						(char)tolower(msg[3])=='t' && (char)tolower(msg[4])==' ' && (char)tolower(msg[5])=='t' && (char)tolower(msg[6])=='o' && 
						(char)tolower(msg[7])==':'){					
					if (index >=13){ // <@>\r\n
						if (status==2){
							msg[index-1] = '\0';
							string to_add(&msg[8]);
							msg[index-1] = '\r';
							to_add.erase(remove_if(to_add.begin(), to_add.end(), Isspace), to_add.end());
							//cout << to_add << endl;
							int len = to_add.length();
							size_t found = to_add.find("@localhost");
							//size_t basic = to_add.find("@");
							// cout << found << endl;
							// cout << len << endl;
							// cout << to_add.substr(1,found-1) << endl;
							if (found!= string::npos && found+11==len){
								do_write(clientfd, response, strlen(response));
								do_write(clientfd, crlf, 2);
								if (debug==1){
									fprintf(stderr, "[%d] %s", clientfd, "S: ");
									fprintf(stderr, "%s\n", response);							
								}
								recipient.push_back(to_add.substr(1,found-1));
								//cout << "entered1" << endl;
								//status = 3;
							}
							// else if(found==string::npos && basic!=string::npos){
							// 	do_write(clientfd, response, strlen(response));
							// 	do_write(clientfd, crlf, 2);
							// 	if (debug==1){
							// 		fprintf(stderr, "[%d] %s", clientfd, "S: ");
							// 		fprintf(stderr, "%s\n", response);							
							// 	}
							// 	unknowns.push_back(to_add.substr(1,to_add.length()-2));
							// }
							else{

								do_write(clientfd, err550, strlen(err550));
								do_write(clientfd, crlf, 2);
								if (debug==1){
									fprintf(stderr, "[%d] %s", clientfd, "S: ");
									fprintf(stderr, "%s\n", err550);																
								}
							}
						}
						else{
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}	
						}
					}
					else{

						if (status==2){
							do_write(clientfd, err501, strlen(err501));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err501);							
							}
						}
						else{
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}
						}
					}
					delete[] msg;
					msg = new char[1000000];
					index = 0;	
				}


					// DATA <CRLF>
				else if (index>4 && (char)tolower(msg[0])=='d' && (char)tolower(msg[1])=='a' && (char)tolower(msg[2])=='t' && 
						(char)tolower(msg[3])=='a' && (char)tolower(msg[4])=='\r') {
											
					if (index >=6){
						
						if (status==3 || (status==2 && recipient.size()>0)){

							
							if (msg[index-2]=='.' && msg[index-3] == '\n'){
								time_t now = time(0);   
   								struct tm * ltime = localtime(&now);
    							string firstline = string("From \0") + sender + string(" \0")+ string(asctime(ltime));
    							msg[index-2] = '\0';
    							string data = string(&msg[6]);
    							//cout << "entered2" << endl;
    							vector <int> recpt_success;
    							
								for (unsigned int idx = 0; idx<recipient.size(); idx++){

									
									request req_addr;
									req_addr.set_request_type("GET");
									req_addr.set_request_num(0);
									req_addr.set_row(recipient[idx]);
									req_addr.set_col("SMTP");

									request recv_addr;
									
									send_recv(recv_addr, req_addr, master_ip, master_port);

									if (recv_addr.val1_size()==0){
										recpt_success.push_back(0);
									}
									else{										
										req_addr.set_col("MAILS/");									
										
										vector<int> fds;
										int max_idx;
										vector<int> versions;
										//cout << col << endl;
										vector<string> replicas;
										for (int z=0; z < recv_addr.val1_size(); z++){
											replicas.push_back(recv_addr.val1(z));
										}

										// ask_version(replicas,recipient[idx],"MAILS/",R,fds,versions,max_idx);
										// request recv_mailList;
										// replicate_read(req_addr, recv_mailList, fds, max_idx);
										// int listsize = recv_mailList.val1_size();
										counter++;
										bool success = replicate_upload(replicas, recipient[idx], "MAILS/", "mail", data,  W, true);

										if (success== true){
											recpt_success.push_back(1);
										}
										else{
											recpt_success.push_back(0);	
										}
									}																
									
								}	

								string failures = "";
								for (unsigned int z = 0; z< recpt_success.size(); z++){
									if (recpt_success[z]==0){
										if (z==0){
											failures = failures + recipient[z];
										}
										else{
											failures = failures + ", " + recipient[z];
										}
									}
								}

								if (failures.compare("")==0){
									do_write(clientfd, response, strlen(response));
									do_write(clientfd, crlf, 2);
									if (debug==1){
										fprintf(stderr, "[%d] %s", clientfd, "S: ");
										fprintf(stderr, "%s\n", response);							
									}
								}
								else{
									string err_rcpt = "553 Transaction Failed for " + failures; 
									do_write(clientfd, err_rcpt.c_str(), strlen(err_rcpt.c_str()));
									do_write(clientfd, crlf, 2);
									if (debug==1){
										fprintf(stderr, "[%d] %s", clientfd, "S: ");
										fprintf(stderr, "%s\n", err_rcpt.c_str());							
									}
								}
															
								
								
								status = 1;
								recipient.clear();
								delete[] msg;
								msg = new char[1000000];
								index = 0;	

							}
							else{
								
								index = index+1;
								status = 3;
							}
							
						}
						else{
							
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}
							delete[] msg;
							msg = new char[1000000];
							index = 0;
						}
					}

					else{
						
						if(status==3 || (status==2 && recipient.size()>0)){
							
							index = index+1;
							status = 3;
							do_write(clientfd, response354, strlen(response354));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", response354);							
							}
														
						}
						else{
							do_write(clientfd, err503, strlen(err503));
							do_write(clientfd, crlf, 2);
							if (debug==1){
								fprintf(stderr, "[%d] %s", clientfd, "S: ");
								fprintf(stderr, "%s\n", err503);							
							}
							delete[] msg;
							msg = new char[1000000];
							index = 0;
						}
					}							
				}


				// RSET
				else if ((char)tolower(msg[0])=='r' && (char)tolower(msg[1])=='s' && (char)tolower(msg[2])=='e' && 
						(char)tolower(msg[3])=='t' && (char)tolower(msg[4])=='\r' && index==5){					
					
					if (status>0){
						do_write(clientfd, response, strlen(response));
						do_write(clientfd, crlf, 2);
						if (debug==1){
							fprintf(stderr, "[%d] %s", clientfd, "S: ");
							fprintf(stderr, "%s\n", response);							
						}

						status = 1;
						recipient.clear();
						
					}
					else{
						do_write(clientfd, err503, strlen(err503));
						do_write(clientfd, crlf, 2);
						if (debug==1){
							fprintf(stderr, "[%d] %s", clientfd, "S: ");
							fprintf(stderr, "%s\n", err503);							
						}
					}
					
					delete[] msg;
					msg = new char[1000000];
					index = 0;	
				}


				// if first 4 characters are "quit"
				else if ((char)tolower(msg[0])=='q' && (char)tolower(msg[1])=='u' && (char)tolower(msg[2])=='i' && (char)tolower(msg[3])=='t'
						 && (char)tolower(msg[4])=='\r' && index==5){
					
					//msg[4] = '\0';
					//cout << msg << endl;	
					do_write(clientfd, response221, strlen(response221));
					do_write(clientfd, crlf, 2);
					if (debug==1){
						fprintf(stderr, "[%d] %s", clientfd, "S: ");
						fprintf(stderr, "%s\n", response221);							
					}

					delete[] msg;

					usleep(1000000);
					close(clientfd);
					erase_from_set(clientfd);					
					pthread_exit(NULL);
					
				}

				else if ((char)tolower(msg[0])=='n' && (char)tolower(msg[1])=='o' && (char)tolower(msg[2])=='o' && (char)tolower(msg[3])=='p'
						 && (char)tolower(msg[4])=='\r' && index==5){
					
					//if(msg[4]==' ' || index==5){ // only if quit is followed by a space or \r\n
						
					do_write(clientfd, response, strlen(response));
					do_write(clientfd, crlf, 2);
					if (debug==1){
						fprintf(stderr, "[%d] %s", clientfd, "S: ");
						fprintf(stderr, "%s\n", response);							
					}

					delete[] msg;
					msg = new char[1000000];
					index = 0;
					
				}
				else{ // Anything other than recognized commands
					do_write(clientfd, err500, strlen(err500));
					do_write(clientfd, crlf, 2);
					if (debug==1){
						fprintf(stderr, "[%d] %s", clientfd, "S: ");
						fprintf(stderr, "%s\n", err500);							
					}
					delete[] msg;
					msg = new char[1000000];
					index = 0;
					
				}

			}
			else if(msg[index]=='\n' && index<=4){ // Anything less than 4 characters

				do_write(clientfd, err500, strlen(err500));
				do_write(clientfd, crlf, 2);
				if (debug==1){
					fprintf(stderr, "[%d] %s", clientfd, "S: ");
					fprintf(stderr, "%s\n", err500);							
				}
				delete[] msg;
				msg = new char[1000000];
				index = 0;
			}
			else {
				index+=n; // If linefeed character is not encountered, continue to read
			}
		}
	}
}

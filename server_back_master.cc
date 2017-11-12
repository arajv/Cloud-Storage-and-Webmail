#include "server_back_master.h"

int main(int argc, char *argv[])
{
	// Reading command line inputs
	if (argc == 1) {
    	fprintf(stderr, "Insufficient Arguments\n");
    	exit(1);
    }    

	char* filename = argv[1];
	ifstream servers(filename);
	string line;
	if (servers.is_open()){
		int i = 0;
		while(getline(servers,line)){			
			if (i==0){					
				get_ip_port (line, ip_addr, port);					
			}
			// else if (i==1){
			// 	//cout << line << endl;
			// 	smtp_server = line;
			// }
			else{
				//cout << line << endl;
				backend_servers.push_back(line);
				//backserver_index[line] = i-1;			

			}			
			i++;
		}
		servers.close();
	}

	// Opening the Socket
	int sockfd = socket(PF_INET,SOCK_STREAM,0);
	if (sockfd<0){
		fprintf(stderr,"%s\n", "Cannot open socket");
		exit(1);
	}
	//cout << sockfd << endl;
	int enable = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	
	sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(port);
	cout << port << endl;
	inet_pton(PF_INET, ip_addr.c_str(), &(server_addr.sin_addr));
	bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	listen(sockfd, 5);


	string file = "checkpoints/checkpoint_master.txt";
	ifstream input (file);

	if (input.fail()==false){
		recover_master(input);
	}
	else{
		assign_servers();
		master_checkpoint();	
	}
	

	

	while(true){		

		sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		int* clientfd = new int;
		*clientfd = accept(sockfd, (sockaddr*)&client_addr, &addrlen);				
		cout << "connection accepted from front end" << endl;
		
		request req_pass;
		int rbyte = readmsg(*clientfd, req_pass);
		if (rbyte==0){
			cout << "0 bytes received from front end" << endl;
			close(*clientfd);
			continue;
		}
		cout << "request received from front end" << endl;
		string user = req_pass.row();
		char first_letter = tolower(user[0]);
		vector<string> assigned_servers = server_assignment[first_letter];
		for (unsigned int j=0; j<assigned_servers.size(); j++){
			req_pass.add_val1(assigned_servers[j]);
			req_pass.add_val2(assigned_servers[j]);					
		}							

		sendmsg(*clientfd, req_pass);
		close(*clientfd);	


	} // While loop ends


} // Main Function ends



void assign_servers(){
	int num = backend_servers.size();
	srand (time(NULL));
	 
	int serv_num;
	
	for (char c = 'a'; c <= 'z'; c++){
		int count = 0;
		unordered_set <int> rand_ints;
    	while (rand_ints.size() < num_replications){

			serv_num = rand() % num;
			rand_ints.insert(serv_num);

			if (rand_ints.size() == count+1){			
				server_assignment[c].push_back(backend_servers[serv_num]);
				count += 1;
			}
		}
		//cout << endl;
    }	
}


void print_things(){
	for (int i = 0; i< backend_servers.size(); i++){
		cout << backend_servers[i] << endl;
	}

	for (char c = 'a'; c <= 'z'; c++){

		cout << c << ": ";
		for (int i=0; i<num_replications; i++){
			cout << server_assignment[c][i] << ", ";
		}
		cout << endl;
	}
}


void master_checkpoint(){
	char folder[] = "checkpoints";
	mkdir(folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	string file = "checkpoints/checkpoint_master.txt";
	ofstream output (file);

	for (char c ='a'; c<= 'z'; c++){
		for (int i=0; i<num_replications; i++){
			if (i==num_replications-1){
				output << server_assignment[c][i];
			}
			else{
				output << server_assignment[c][i] << ",";
			}
		}
		output << "\n";
	} 
	output.close();
}


void recover_master(ifstream& input){
	
	string line;
	int pos;
	int start;

	for (char c ='a'; c<= 'z'; c++){
		getline(input,line);
		start = 0;
		//cout << c << ": ";
		for (int i=0; i<num_replications; i++){
			if (i==num_replications-1){				
				server_assignment[c].push_back(line.substr(start, line.length()-start));
			}
			else{
				pos = line.find(",", start);
				server_assignment[c].push_back(line.substr(start, pos-start));
				start = pos+1;
			}
			//cout  << server_assignment[c][i] << ",";
		}
		//cout << endl;		
	}

	input.close(); 
}
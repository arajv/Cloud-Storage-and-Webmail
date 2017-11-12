/*
 * serverbck.cpp
 *
 *  Created on: Apr 10, 2017
 *      Author: cis505
 */
#include "server_bck.hh"
#include "process_operation.hh"
#include "process_req_bck.hh"
#include "fault_tolerance.hh"

unordered_map<string,map<string,vector<string>>> key_val_store;
unordered_map<string,pthread_mutex_t> locks;
pthread_rwlock_t masterlock = PTHREAD_RWLOCK_INITIALIZER;
server_addr mstr_srv_addr;
server_addr my_addr;
vector <server_addr> other_servers;
int server_num;
bool verbose = false;
int listen_fd;

int main(int argc, char *argv[]){

	int opt;
	extern char *optarg;
	extern int optind;
	while( (opt = getopt( argc, argv, "v")) != EOF ) {
		switch(opt){
			case 'v':
				verbose = true;
				break;
			default:
			    exit(0);
		}
	}

    if (optind+1>=argc){
            cerr<<"No file name given"<<endl;
            exit(-1);
    }

    ifstream addr_file(argv[optind]);
    string line;
    getline(addr_file,line);
    int colon_pos = line.find(':');
    mstr_srv_addr.ip = line.substr(0,colon_pos);
    mstr_srv_addr.port = stoi(line.substr(colon_pos+1));

    server_num = atoi(argv[optind + 1]);
    int lineNum = 0;
    while(getline(addr_file,line)){
    	lineNum++;
    	colon_pos = line.find(':');
    	if(lineNum == server_num){
    		my_addr.ip = line.substr(0,colon_pos);
    		my_addr.port = stoi(line.substr(colon_pos+1));
    		break;
    	}
    	else{
    		server_addr s_addr;
    		s_addr.ip = line.substr(0,colon_pos);
    		s_addr.port = stoi(line.substr(colon_pos+1));
    		other_servers.push_back(s_addr);
    	}
    }

    make_necessary_folders();

    ifstream recover_cp1 ("checkpoints/checkpoint" + to_string(server_num) + "_temp" +  ".txt");
    ifstream recover_cp2 ("checkpoints/checkpoint" + to_string(server_num) + ".txt");
    
    if (recover_cp1.fail()== false){
    	recover_from_checkpoint(recover_cp1);
    	log_recovery(true);
    	updates_from_backends();
    }
    else if(recover_cp2.fail()== false){
    	recover_from_checkpoint(recover_cp2);
    	log_recovery(false);
    	updates_from_backends();	
    }
    else{
    	vector<string> pass{"ss"};
	    insert_elem("oshin","PASSWORD",pass);
	    vector<string> mails{"mail1", "mail2", "mail3"};
	    insert_elem("oshin","MAILS/",mails);
	    // insert_elem("oshin","MAILS/","mail2");
	    // insert_elem("oshin","MAILS/","mail3");
	    vector<string> empty;
	    insert_elem("oshin","/",empty);
	    insert_elem("oshin","TRASH/",empty);
	    vector<string> v{"1"};
	    insert_elem("oshin","version/MAILS/",v);
	    insert_elem("oshin","version/TRASH/",v);
	    insert_elem("oshin","version/MAILS/mail1",v);
	    insert_elem("oshin","version/MAILS/mail2",v);
	    insert_elem("oshin","version/MAILS/mail3",v);
	    insert_elem("oshin","version//",v);
	    insert_elem("oshin","version/PASSWORD",v);
	    vector<string> mail1{"From: <arajv@localhost>\r\nSubject: Mail1\r\nBlah\r\n"};
	    vector<string> mail2{"From: <arajv@localhost>\r\nSubject: Mail2\r\nBlah\r\n"};
	    vector<string> mail3{"From: <arajv@localhost>\r\nSubject: Mail3\r\nBlah\r\n"};
	    insert_elem("oshin","MAILS/mail1",mail1);
	    insert_elem("oshin","MAILS/mail2",mail2);
	    insert_elem("oshin","MAILS/mail3",mail3);

	    insert_elem("oshin", "TRASH/", empty);
	    insert_elem("oshin", "version/TRASH/", v);

	    insert_elem("oshin", "CONTACTS", empty);
	    insert_elem("oshin", "version/CONTACTS", v);

	    checkpoint();
	}
    
    listen_for_connections();

    launch_checkpointing_thread();

    accept_new_connections();

	return 0;
}

void listen_for_connections(){

	listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(my_addr.ip.c_str());
	servaddr.sin_port = htons(my_addr.port);

	int ret;
	int enable = 1;

	ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	if(ret == -1){
		cerr<<"setsockopt(SO_REUSEADDR) failed"<<endl;
	}

	ret = bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	if(ret == -1){
		cerr<<"Unable to bind socket. Please try again later"<<endl;
	    exit(0);
	}

	listen(listen_fd, 100);

}

void accept_new_connections(){

	pthread_attr_t attr;
    pthread_attr_init(&attr);
    vector<pthread_t> all_threads;

	while(true){

		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int *fd = (int*)malloc(sizeof(int));
		*fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);

		//cout << "Accepted : " << to_string(ntohs(clientaddr.sin_port)) << " " << *fd << endl;
		pthread_t thread;
		pthread_create(&thread, &attr, worker, fd);
		all_threads.push_back(thread);
	}

	for(int i=0; i<all_threads.size(); i++){
		pthread_join(all_threads[i],NULL);
	}
}

void * worker(void *arg){

	int comm_fd = *(int*)arg;
	//cout << "fd = " << comm_fd << endl;
	//int lock_taken = 0;
		
	request req1; // normally for asking version number 
	int r = readmsg(comm_fd, req1);
	// if (req1.request_type().compare("")==0){
	// 	close(comm_fd);
	// 	pthread_exit(NULL);
	// }
	pthread_rwlock_rdlock(&masterlock);
	
	if (req1.has_row()==true) {	
		// cout << req1.request_type() << endl;
		// cout << req1.col() << endl;
		// cout << req1.val1_size() << endl;
		// if (req1.val1_size()>0){
		// 	cout << req1.val1(0) << endl;
		// }
		if(locks.find(req1.row()) == locks.end()){
			locks[req1.row()] = PTHREAD_MUTEX_INITIALIZER;	
		} 
		pthread_mutex_lock(&locks[req1.row()]);
		
	}

	string logfile;

	if (req1.has_row()==true){
		logfile = "logs/"+ to_string(server_num) + "/" + req1.row() + ".txt";
		log_request(req1, logfile);
	}

	request res1 = process_request(req1);
	//cout << res1.request_type() << endl;
	//cout << res1.col() << endl;
	 //cout << res1.val1_size() << endl;
	 //if (res1.val1_size()>0){
	 //	cout << res1.val1(0) << endl;
	 //}
	int s = sendmsg(comm_fd,res1);

	if (req1.request_num()==0){
		if (req1.has_row()==true){
			//logfile.close();
			pthread_mutex_unlock(&locks[req1.row()]);
		}
		pthread_rwlock_unlock(&masterlock);
		//usleep(10000000);
		close(comm_fd);
		pthread_exit(NULL);
	}

	request req2;
	r = readmsg(comm_fd, req2);

	if (r==0){
		if (req1.has_row()==true){
			//logfile.close();
			pthread_mutex_unlock(&locks[req1.row()]);
		}
		pthread_rwlock_unlock(&masterlock);
		close(comm_fd);
		pthread_exit(NULL);
	}

	if (req1.has_row()==true){
		logfile = "logs/"+ to_string(server_num) + "/" + req1.row() + ".txt";
		log_request(req2, logfile);
	}
	
	request res2 = process_request(req2);
	s = sendmsg(comm_fd, res2);

	if (req2.request_num()==0){
		if (req1.has_row()==true){
			//logfile.close();
			pthread_mutex_unlock(&locks[req1.row()]);
		}
		pthread_rwlock_unlock(&masterlock);
		close(comm_fd);
		pthread_exit(NULL);
	}	

	while (true){
		request req;
		r = readmsg(comm_fd, req);

		if (r==0){
			if (req1.has_row()==true){
				//logfile.close();
				pthread_mutex_unlock(&locks[req1.row()]);
			}
			pthread_rwlock_unlock(&masterlock);
			close(comm_fd);
			pthread_exit(NULL);
		}			

		if (req1.has_row()==true){
			logfile = "logs/"+ to_string(server_num) + "/" + req1.row() + ".txt";
			log_request(req, logfile);
		}

		request res = process_request(req);
		s = sendmsg(comm_fd, res);

		if (req.request_num()==0){
			if (req1.has_row()==true){
				//logfile.close();
				pthread_mutex_unlock(&locks[req1.row()]);
			}
			break;
		}			
	}
	
	pthread_rwlock_unlock(&masterlock);
	close(comm_fd);
	pthread_exit(NULL);	
	
}

void launch_checkpointing_thread(){
	pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_t thread;
	pthread_create(&thread, &attr, chkpnt_worker, NULL);
}

void* chkpnt_worker(void *arg){
	usleep(120000000);
	while(true){
		pthread_rwlock_wrlock(&masterlock);

		string name = "checkpoints/checkpoint" + to_string(server_num) + ".txt";
		string tempname = "checkpoints/checkpoint" + to_string(server_num) + "_temp" + ".txt";
		rename(name.c_str(), tempname.c_str());

		DIR* dir;
		dirent *ent;
		string directory = "logs/" + to_string(server_num);
		
		vector <string> tempfiles;

		if ((dir = opendir (directory.c_str())) != NULL) {	  
			int i=0;	
		  	while ((ent = readdir (dir)) != NULL) {
		  		if (ent->d_name[0]!= '.'){
		  			//files.insert(string(ent->d_name));	  			
		  			string file = directory+"/"+string(ent->d_name);
		  			string file_temp = directory + "/temp" +"/" + string(ent->d_name);
		  			rename(file.c_str(), file_temp.c_str());
		  			tempfiles.push_back(file_temp);	    		
		    	}
		  	}
		  	closedir (dir);
		}

		checkpoint();
		remove(tempname.c_str());

		for (unsigned int i=0; i<tempfiles.size(); i++){
			remove(tempfiles[i].c_str());
		}

		// DIR* dir;
		// dirent *ent;
		// string directory = "logs/" + to_string(server_num);		

		// if ((dir = opendir (directory.c_str())) != NULL) {	  
		// 	int i=0;	
		//   	while ((ent = readdir (dir)) != NULL) {
		//   		if (ent->d_name[0]!= '.'){
		//   			string file = directory+"/"+string(ent->d_name);
		//   			remove(file.c_str());
		//   		}
		//   	}
		//   	closedir (dir);
		// }
		
		pthread_rwlock_unlock(&masterlock);
		usleep(180000000);
	}
}

void log_recovery(bool temp_recovery){
	DIR* dir;
	dirent *ent;
	string directory = "logs/" + to_string(server_num);
	//vector <string> files;

	if ((dir = opendir (directory.c_str())) != NULL) {	  
		int i=0;	
	  	while ((ent = readdir (dir)) != NULL) {
	  		if (ent->d_name[0]!= '.'){
	  			//files.insert(string(ent->d_name));	  			
	  			recover_from_log(directory+"/"+string(ent->d_name));	    		
	    	}
	  	}
	  	closedir (dir);
	}

	if (temp_recovery = true){
		directory = "logs/" + to_string(server_num) + "/temp";
		if ((dir = opendir (directory.c_str())) != NULL) {	  
			int i=0;	
		  	while ((ent = readdir (dir)) != NULL) {
		  		if (ent->d_name[0]!= '.'){
		  			//files.insert(string(ent->d_name));	  			
		  			recover_from_log(directory+"/"+string(ent->d_name));	    		
		    	}
		  	}
		  	closedir (dir);
		}
	}
	
}

void make_necessary_folders(){
	char folder1[] = "checkpoints";
	mkdir(folder1, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	char folder2[] = "logs";
	mkdir(folder2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	string folder3 = "logs/" + to_string(server_num);
	mkdir(folder3.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	string folder4 = folder3 + "/temp";
	mkdir(folder4.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void test_element(){

	/*request request_test,result;
	request_test.set_request_type("GET_COLS");
	request_test.set_request_num(0);
	request_test.set_row("oshin");
	result = process_request(request_test);
	cout<<result.val1_size()<<endl;
	cout<<result.val1(0);
	insert_elem("oshin","mail:/mail1","oshin agarwal mail");
	print_map();
	insert_elem("oshin","file:/file1","oshin agarwal file");
	print_map();
	insert_elem("abhinav","file:/file1","arajv file");
	print_map();
	insert_elem("abhinav","file:/file2","arajv file2");
	print_map();
	delete_elem("abhinav","file:/file1");
	print_map();
	replace_elem("oshin","mail:/mail1","oshin agarwal mail","oshin agarwal mail22");
	print_map();
	cout<<"Reading...";
	cout<<read_elem("oshin","mail:/mail1");
	cout<<endl;*/
	print_map();
}

void print_map(){

	/*for(unordered_map<string,map<string,string>>::iterator it = key_val_store.begin();it!=key_val_store.end();it++){
		cout<<it->first<<endl;
		map<string,string>::iterator it2 = it->second.begin();
		while(it2!=it->second.end()){
			cout<<it2->first<<" ---- "<<it2->second<<endl;
			it2++;
		}
	}*/
	cout<<endl;
}


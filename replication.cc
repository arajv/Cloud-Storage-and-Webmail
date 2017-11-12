#include "replication.h"

// Ask versions of a given column to the replicas of the user
bool ask_version(const vector<string>& replicas, const string& row, const string& col, int quorum, vector<int>& fds, vector<int>& versions, int& max_idx, bool is_frontend){
					
	request req_version;
	req_version.set_request_type("GET");
	req_version.set_request_num(1);
	req_version.set_row(row);
	req_version.set_col("version/"+ col);
	//cout << "version/"+ col << endl;
	fd_set rfds;
	//vector <int> fds;
	int fd_range = 0;
	//vector <int> versions;
	
	for (unsigned int i=0; i<replicas.size(); i++){
		string ip;
		int port;
		get_ip_port(replicas[i], ip, port);
		int sockfd = socket(PF_INET, SOCK_STREAM, 0);

		sockaddr_in serv_addr;
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = PF_INET;
		serv_addr.sin_port = htons(port);				
		inet_pton(PF_INET, ip.c_str(), &(serv_addr.sin_addr));
		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==0){
			//FD_SET(sockfd, &rfds);
			fds.push_back(sockfd);
			versions.push_back(0);
			fd_range = max(fd_range, sockfd+1);
			sendmsg(sockfd, req_version);		 				
		}
		else{
			fprintf(stderr, "%s\n", "Error in connecting to server");
		}					
	}

	if (is_frontend == true){
		if (fds.size() < quorum){
			close_fds(fds);
			return false;
		}
	}

	if (fds.size() == 0){
		return false;
	}

	int max_version = -1;
	//int max_idx;
	int count = 0;

	struct timeval timeout;
	timeout.tv_sec = deadlock_wait;
	timeout.tv_usec = 0;
	
	while(true){

		FD_ZERO(&rfds);
		for (unsigned int i=0; i< fds.size(); i++){
			FD_SET(fds[i], &rfds);
		}
		int s = select(fd_range, &rfds, NULL, NULL, &timeout);

		if (s==0){
			close_fds(fds);
			return false;
		}
		
		for (unsigned int i=0; i< fds.size(); i++){
			//sendmsg(fds[i], req_version);
			if (FD_ISSET(fds[i], &rfds)){
				request recv_version;
				readmsg(fds[i], recv_version);
				// int version = stoi(recv_version.val1(0));
				// versions[i] = version;
				int version;
				if (recv_version.val1_size()>0){
					version = stoi(recv_version.val1(0));
					versions[i] = version;
				}
				else{
					version = -1;
					versions[i] = -1; 
				}
				// versions.push_back(version);
				// final_fds.push_back(fds[i]);
				max_version = max(max_version, version);
				if (max_version==version){
					max_idx = i;
				}
				count+=1;				
			}	
		}

		if (count == fds.size()){
			break;
		}
		// if (is_frontend == true){
		// 	if (count >= quorum){
		// 		break;
		// 	}
		// }
		// else{
		// 	if (count >= min(quorum,(int)fds.size())){
		// 		break;
		// 	}
		// }
	}

	if (max_version== -1){
		
		close_fds(fds);
		return false;
	}

	return true;
}





// Ask the server (which has the highest version number) for the data 
void replicate_read(const request& req, request& req_recv, const vector<int>& fds, int max_idx){
	for (unsigned int i=0; i< fds.size(); i++){
		if (i == max_idx){		
			sendmsg(fds[i], req);
			readmsg(fds[i], req_recv);			
		}
		close(fds[i]);
	}
}



void replicate_delete(const string& user, const string& pfolder, const string& filename, const vector<int>& fds, const vector<int>& versions, int max_idx){
	
	
	vector<int> folder_versions;
	int max_idx_folder;
	ask_pfolder_version(fds, user, pfolder, versions, folder_versions, max_idx_folder);

	request get_folder_contents;
	get_folder_contents.set_request_type("GET");
	get_folder_contents.set_request_num(1);
	get_folder_contents.set_row(user);
	get_folder_contents.set_col(pfolder);

	request req_recv;
	sendmsg(fds[max_idx_folder], get_folder_contents);
	readmsg(fds[max_idx_folder], req_recv);

	vector<string> folder_contents;
	for (int i=0; i<req_recv.val1_size(); i++){
		if (req_recv.val1(i).compare(filename)!=0){
			folder_contents.push_back(req_recv.val1(i));
		}
	}

	request update_folder;
	update_folder.set_request_type("CPUT");
	update_folder.set_request_num(1);
	update_folder.set_row(user);
	update_folder.set_col(pfolder);
	for (unsigned int i=0; i< folder_contents.size(); i++){
		update_folder.add_val2(folder_contents[i]);
	}

	request update_folder_version;
	update_folder_version.set_request_type("CPUT");
	update_folder_version.set_request_num(1);
	update_folder_version.set_row(user);
	update_folder_version.set_col("version/"+pfolder);
	update_folder_version.add_val2(to_string(folder_versions[max_idx_folder]+1));

	request delete_file;
	delete_file.set_request_type("DELETE");
	delete_file.set_request_num(1);
	delete_file.set_row(user);
	delete_file.set_col(pfolder + filename);

	request update_file_version;
	update_file_version.set_request_type("CPUT");
	update_file_version.set_request_num(0);
	update_file_version.set_row(user);
	update_file_version.set_col("version/"+pfolder+filename);
	update_file_version.add_val2(to_string(versions[max_idx]+1));

	
	
	
	for (unsigned int idx=0; idx< fds.size(); idx++){
		if (versions[idx]>0){
			request req_recv1;			
					
			sendmsg(fds[idx], get_folder_contents);
			readmsg(fds[idx], req_recv1);

			if (update_folder.val1_size()>0){
				update_folder.clear_val1();
			}

			for (int i=0; i< req_recv1.val1_size(); i++){
				update_folder.add_val1(req_recv1.val1(i)); 	
			}

			request req_recv2;
			sendmsg(fds[idx], update_folder);
			readmsg(fds[idx], req_recv2);

			if (update_folder_version.val1_size()>0){
				update_folder_version.clear_val1();
			}
			update_folder_version.add_val1(to_string(folder_versions[idx]));
			request req_recv3;
			sendmsg(fds[idx], update_folder_version);
			readmsg(fds[idx], req_recv3);			

			request req_recv4;	
			sendmsg(fds[idx], delete_file);
			readmsg(fds[idx], req_recv4);

			if (update_file_version.val1_size()>0){
				update_file_version.clear_val1();
			}
			update_file_version.add_val1(to_string(versions[idx]));
			request req_recv5;
			sendmsg(fds[idx], update_file_version);
			readmsg(fds[idx], req_recv5);
		}
		close(fds[idx]);
	}
	
	//return true;
	
}


bool replicate_rename(const string& user, const string& pfolder, const string& filename, const string& newname, const vector<int>& fds, const vector<int>& versions, int max_idx){
	
	vector<int> folder_versions;
	int max_idx_folder;
	ask_pfolder_version(fds, user, pfolder, versions, folder_versions, max_idx_folder);

	request get_folder_contents;
	get_folder_contents.set_request_type("GET");
	get_folder_contents.set_request_num(1);
	get_folder_contents.set_row(user);
	get_folder_contents.set_col(pfolder);

	request req_recv;
	sendmsg(fds[max_idx_folder], get_folder_contents);
	readmsg(fds[max_idx_folder], req_recv);

	

	int file_exists = 0;
	vector<string> folder_contents;
	for (int i=0; i<req_recv.val1_size(); i++){
		if (req_recv.val1(i).compare(newname)==0){
			close_fds(fds);
			return false;
		}
		if (req_recv.val1(i).compare(filename)!=0){
			folder_contents.push_back(req_recv.val1(i));
		}
		else{
			file_exists = 1;
			folder_contents.push_back(newname);
		}
	}
	if (file_exists==0){
		close_fds(fds);
		return false;
	}

	request update_folder;
	update_folder.set_request_type("CPUT");
	update_folder.set_request_num(1);
	update_folder.set_row(user);
	update_folder.set_col(pfolder);
	for (unsigned int i=0; i< folder_contents.size(); i++){
		update_folder.add_val2(folder_contents[i]);
	}

	request update_folder_version;
	update_folder_version.set_request_type("CPUT");
	update_folder_version.set_request_num(0);
	update_folder_version.set_row(user);
	update_folder_version.set_col("version/"+pfolder);
	update_folder_version.add_val2(to_string(folder_versions[max_idx_folder]+1));

	request get_file;
	get_file.set_request_type("GET");
	get_file.set_request_num(1);
	get_file.set_row(user);	
	get_file.set_col(pfolder+filename);

	request delete_file;
	delete_file.set_request_type("DELETE");
	delete_file.set_request_num(1);
	delete_file.set_row(user);
	delete_file.set_col(pfolder + filename);

	request update_file_version;
	update_file_version.set_request_type("CPUT");
	update_file_version.set_request_num(1);
	update_file_version.set_row(user);
	update_file_version.set_col("version/"+pfolder+filename);
	update_file_version.add_val2(to_string(versions[max_idx]+1));

	request new_file;
	new_file.set_request_type("PUT");
	new_file.set_request_num(1);
	new_file.set_row(user);	
	new_file.set_col(pfolder + newname);

	request new_version;
	new_version.set_request_type("PUT");
	new_version.set_request_num(1);
	new_version.set_row(user);
	new_version.set_col("version/"+ pfolder + newname);
	new_version.add_val1("1");

	
	
	for (unsigned int idx=0; idx< fds.size(); idx++){
		if (versions[idx]>0){
			request req_recv1;
			sendmsg(fds[idx], get_file);
			readmsg(fds[idx], req_recv1);
			string file_content = req_recv1.val1(0);

			request req_recv2;
					
			sendmsg(fds[idx], delete_file);
			readmsg(fds[idx], req_recv2);

			if (update_file_version.val1_size()>0){
				update_file_version.clear_val1();
			}
			request req_recv3;
			update_file_version.add_val1(to_string(versions[idx]));		
			sendmsg(fds[idx], update_file_version);
			readmsg(fds[idx], req_recv3);

			if (new_file.val1_size()>0){
				new_file.clear_val1();
			}
			request req_recv4;
			new_file.add_val1(file_content);
			sendmsg(fds[idx], new_file);
			readmsg(fds[idx], req_recv4);

			request req_recv5;
			sendmsg(fds[idx], new_version);
			readmsg(fds[idx], req_recv5);

			request req_recv6;			
					
			sendmsg(fds[idx], get_folder_contents);
			readmsg(fds[idx], req_recv6);

			if (update_folder.val1_size()>0){
				update_folder.clear_val1();
			}

			for (int i=0; i< req_recv6.val1_size(); i++){
				update_folder.add_val1(req_recv6.val1(i)); 	
			}

			request req_recv7;
			sendmsg(fds[idx], update_folder);
			readmsg(fds[idx], req_recv7);

			if (update_folder_version.val1_size()>0){
				update_folder_version.clear_val1();
			}
			update_folder_version.add_val1(to_string(folder_versions[idx]));
			request req_recv8;
			sendmsg(fds[idx], update_folder_version);
			readmsg(fds[idx], req_recv8);

		}
		close(fds[idx]);
	}
	
	return true;
	
}


bool replicate_move(const string& user, const string& pfolder1, const string& pfolder2, const string& filename, const vector<int>& fds, const vector<int>& versions, int max_idx){
	
	vector<int> folder_versions1;
	int max_idx_folder1;
	bool b1 = ask_pfolder_version(fds, user, pfolder1, versions, folder_versions1, max_idx_folder1);
	if (b1==false){
		return false;
	}

	vector<int> folder_versions2;
	int max_idx_folder2;
	bool b2 = ask_pfolder_version(fds, user, pfolder2, versions, folder_versions2, max_idx_folder2);
	if (b2==false){
		return false;
	}

	request get_folder_contents1;
	get_folder_contents1.set_request_type("GET");
	get_folder_contents1.set_request_num(1);
	get_folder_contents1.set_row(user);
	get_folder_contents1.set_col(pfolder1);

	request req_recv11;
	sendmsg(fds[max_idx_folder1], get_folder_contents1);
	readmsg(fds[max_idx_folder1], req_recv11);

	if (req_recv11.val1_size()==0 && pfolder1.compare("/")!=0){
		close_fds(fds);
		return false;
	}

	vector<string> folder_contents1;
	int file_exists = 0;
	for (int i=0; i<req_recv11.val1_size(); i++){
		if (req_recv11.val1(i).compare(filename)!=0){
			folder_contents1.push_back(req_recv11.val1(i));
		}
		else{
			file_exists=1;		
		}		
	}
	if (file_exists==0){
		close_fds(fds);
		return false;
	}

	request get_folder_contents2;
	get_folder_contents2.set_request_type("GET");
	get_folder_contents2.set_request_num(1);
	get_folder_contents2.set_row(user);
	get_folder_contents2.set_col(pfolder2);

	request req_recv22;
	sendmsg(fds[max_idx_folder2], get_folder_contents2);
	readmsg(fds[max_idx_folder2], req_recv22);
	if (req_recv22.val1_size()==0 && pfolder2.compare("/")!=0 && pfolder2.compare("TRASH/")!=0 && pfolder2.compare("MAILS/")!=0){
		close_fds(fds);
		return false;
	}

	vector<string> folder_contents2;
	for (int i=0; i<req_recv22.val1_size(); i++){
		if (req_recv22.val1(i).compare(filename)==0){
			close_fds(fds);
			return false;
		}		
		folder_contents2.push_back(req_recv22.val1(i));				
	}
	folder_contents2.push_back(filename);

	request update_folder1;
	update_folder1.set_request_type("CPUT");
	update_folder1.set_request_num(1);
	update_folder1.set_row(user);
	update_folder1.set_col(pfolder1);
	for (unsigned int i=0; i< folder_contents1.size(); i++){
		update_folder1.add_val2(folder_contents1[i]);
	}

	request update_folder2;
	update_folder2.set_request_type("CPUT");
	update_folder2.set_request_num(1);
	update_folder2.set_row(user);
	update_folder2.set_col(pfolder2);
	for (unsigned int i=0; i< folder_contents2.size(); i++){
		update_folder2.add_val2(folder_contents2[i]);
	}

	request update_folder_version1;
	update_folder_version1.set_request_type("CPUT");
	update_folder_version1.set_request_num(1);
	update_folder_version1.set_row(user);
	update_folder_version1.set_col("version/"+pfolder1);
	update_folder_version1.add_val2(to_string(folder_versions1[max_idx_folder1]+1));

	request update_folder_version2;
	update_folder_version2.set_request_type("CPUT");
	update_folder_version2.set_request_num(0);
	update_folder_version2.set_row(user);
	update_folder_version2.set_col("version/"+pfolder2);
	update_folder_version2.add_val2(to_string(folder_versions2[max_idx_folder2]+1));


	request get_file;
	get_file.set_request_type("GET");
	get_file.set_request_num(1);
	get_file.set_row(user);
	//get_file.set_col(pfolder1 +"/"+filename);
	get_file.set_col(pfolder1 +filename);

	request delete_file;
	delete_file.set_request_type("DELETE");
	delete_file.set_request_num(1);
	delete_file.set_row(user);
	delete_file.set_col(pfolder1 + filename);

	request update_file_version;
	update_file_version.set_request_type("CPUT");
	update_file_version.set_request_num(1);
	update_file_version.set_row(user);
	update_file_version.set_col("version/"+pfolder1+filename);
	update_file_version.add_val2(to_string(versions[max_idx]+1));

	request new_file;
	new_file.set_request_type("PUT");
	new_file.set_request_num(1);
	new_file.set_row(user);
	new_file.set_col(pfolder2 + filename);

	request new_version;
	new_version.set_request_type("PUT");
	new_version.set_request_num(1);
	new_version.set_row(user);
	new_version.set_col("version/"+ pfolder2 + filename);
	new_version.add_val1("1");

		
	
	for (unsigned int idx=0; idx< fds.size(); idx++){
		if (versions[idx]>0){
			request req_recv1;
			sendmsg(fds[idx], get_file);
			readmsg(fds[idx], req_recv1);
			string file_content = req_recv1.val1(0);

			request req_recv2;
			//delete_file.set_col(pfolder1+ "/" + filename);
					
			sendmsg(fds[idx], delete_file);
			readmsg(fds[idx], req_recv2);

			if (update_file_version.val1_size()>0){
				update_file_version.clear_val1();
			}
			request req_recv3;
			update_file_version.add_val1(to_string(versions[idx]));		
			sendmsg(fds[idx], update_file_version);
			readmsg(fds[idx], req_recv3);

			if (new_file.val1_size()>0){
				new_file.clear_val1();
			}
			request req_recv4;
			new_file.add_val1(file_content);
			sendmsg(fds[idx], new_file);
			readmsg(fds[idx], req_recv4);

			request req_recv5;
			sendmsg(fds[idx], new_version);
			readmsg(fds[idx], req_recv5);

			request req_recv6;			
					
			sendmsg(fds[idx], get_folder_contents1);
			readmsg(fds[idx], req_recv6);

			if (update_folder1.val1_size()>0){
				update_folder1.clear_val1();
			}

			for (int i=0; i< req_recv6.val1_size(); i++){
				update_folder1.add_val1(req_recv6.val1(i)); 	
			}

			request req_recv7;
			sendmsg(fds[idx], update_folder1);
			readmsg(fds[idx], req_recv7);

			if (update_folder_version1.val1_size()>0){
				update_folder_version1.clear_val1();
			}
			update_folder_version1.add_val1(to_string(folder_versions1[idx]));
			request req_recv8;
			sendmsg(fds[idx], update_folder_version1);
			readmsg(fds[idx], req_recv8);

			request req_recv9;			
					
			sendmsg(fds[idx], get_folder_contents2);
			readmsg(fds[idx], req_recv9);

			if (update_folder2.val1_size()>0){
				update_folder2.clear_val1();
			}

			for (int i=0; i< req_recv9.val1_size(); i++){
				update_folder2.add_val1(req_recv9.val1(i)); 	
			}

			request req_recv10;
			sendmsg(fds[idx], update_folder2);
			readmsg(fds[idx], req_recv10);

			if (update_folder_version2.val1_size()>0){
				update_folder_version2.clear_val1();
			}
			update_folder_version2.add_val1(to_string(folder_versions2[idx]));
			request req_recv11;
			sendmsg(fds[idx], update_folder_version2);
			readmsg(fds[idx], req_recv11);


		}
		close(fds[idx]);
	}
	
	return true;
	
}


bool replicate_upload(const vector<string>& replicas, const string& user, const string& pfolder, const string& filename, const string& content,  int quorum, bool mail){
	
	vector<int> fds;
	int max_idx;
	vector<int> versions;
	
	bool b = ask_version(replicas,user,pfolder, quorum, fds, versions, max_idx);

	
	if (b==false){
		return false;
	}

	
	request get_folder_contents;
	get_folder_contents.set_request_type("GET");
	get_folder_contents.set_request_num(1);
	get_folder_contents.set_row(user);
	get_folder_contents.set_col(pfolder);

	request req_recv;
	sendmsg(fds[max_idx], get_folder_contents);
	readmsg(fds[max_idx], req_recv);

	if (mail==false && req_recv.val1_size()==0 && pfolder.compare("/")!=0){
		close_fds(fds);
		return false;
	}

	string filenum = "";

	if (mail==true){
		filenum = to_string(req_recv.val1_size()+1);
	}

	vector<string> folder_contents;
	for (int i=0; i<req_recv.val1_size(); i++){	
		if (mail==false && req_recv.val1(i).compare(filename + filenum)==0){
			close_fds(fds);
			return false;
		}	
		folder_contents.push_back(req_recv.val1(i));	
	}

	
	folder_contents.push_back(filename + filenum);

	request update_folder;
	update_folder.set_request_type("CPUT");
	update_folder.set_request_num(1);
	update_folder.set_row(user);
	update_folder.set_col(pfolder);
	for (unsigned int i=0; i< folder_contents.size(); i++){
		update_folder.add_val2(folder_contents[i]);
	}


	request update_folder_version;
	update_folder_version.set_request_type("CPUT");
	update_folder_version.set_request_num(0);
	update_folder_version.set_row(user);
	update_folder_version.set_col("version/"+pfolder);	
	update_folder_version.add_val2(to_string(versions[max_idx]+1));



	request new_file;
	new_file.set_request_type("PUT");
	new_file.set_request_num(1);
	new_file.set_row(user);
	//new_file.set_col(pfolder + "/" + newname);
	new_file.set_col(pfolder + filename + filenum);
	new_file.add_val1(content);

	request new_version;
	new_version.set_request_type("PUT");
	new_version.set_request_num(1);
	new_version.set_row(user);
	//new_version.set_col("version/"+ pfolder + "/" + newname);
	new_version.set_col("version/"+ pfolder + filename + filenum);
	new_version.add_val1("1");

	
	
	for (unsigned int idx=0; idx< fds.size(); idx++){					
		if (versions[idx]>0){
			request req_recv1;
			sendmsg(fds[idx], new_file);
			readmsg(fds[idx], req_recv1);

			request req_recv2;
			sendmsg(fds[idx], new_version);
			readmsg(fds[idx], req_recv2);

			
			request req_recv4;			
					
			sendmsg(fds[idx], get_folder_contents);
			readmsg(fds[idx], req_recv4);

			if (update_folder.val1_size()>0){
				update_folder.clear_val1();
			}

			for (int i=0; i< req_recv4.val1_size(); i++){
				update_folder.add_val1(req_recv4.val1(i)); 	
			}

			
			request req_recv5;
			sendmsg(fds[idx], update_folder);
			readmsg(fds[idx], req_recv5);

			if (update_folder_version.val1_size()>0){
				update_folder_version.clear_val1();
			}
			update_folder_version.add_val1(to_string(versions[idx]));

			request req_recv6;
			sendmsg(fds[idx], update_folder_version);
			readmsg(fds[idx], req_recv6);

		}
		
		close(fds[idx]);
	}
	
	return true;
	
}


bool replicate_put(const vector<string> replicas, const string& user, const string& filename, const string& content, int quorum){
	
	request req_version;
	req_version.set_request_type("GET");
	req_version.set_request_num(1);
	req_version.set_row(user);
	req_version.set_col("version/"+ filename);
	//cout << "version/"+ col << endl;
	fd_set rfds;
	vector <int> fds;
	int fd_range = 0;
	vector <int> versions;


	
	for (unsigned int i=0; i<replicas.size(); i++){
		string ip;
		int port;
		get_ip_port(replicas[i], ip, port);
		int sockfd = socket(PF_INET, SOCK_STREAM, 0);

		sockaddr_in serv_addr;
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = PF_INET;
		serv_addr.sin_port = htons(port);				
		inet_pton(PF_INET, ip.c_str(), &(serv_addr.sin_addr));
		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==0){
			//FD_SET(sockfd, &rfds);
			fds.push_back(sockfd);
			versions.push_back(0);
			fd_range = max(fd_range, sockfd+1);
			sendmsg(sockfd, req_version);		 				
		}
		else{
			fprintf(stderr, "%s\n", "Error in connecting to server");
		}					
	}

	if (fds.size() < quorum){
		close_fds(fds);
		return false;
	}
	
	
	
	int max_version = -1;
	int max_idx;
	int count = 0;
	struct timeval timeout;
	timeout.tv_sec = deadlock_wait;
	timeout.tv_usec = 0;	

	while(true){

		FD_ZERO(&rfds);
		for (unsigned int i=0; i< fds.size(); i++){
			FD_SET(fds[i], &rfds);
		}

		int s = select(fd_range, &rfds, NULL, NULL, &timeout);
		//cout << "s: " << s << endl;
		if (s==0){
			close_fds(fds);
			return false;
		}
		for (unsigned int i=0; i< fds.size(); i++){
			//sendmsg(fds[i], req_version);
			if (FD_ISSET(fds[i], &rfds)){
				request recv_version;
				readmsg(fds[i], recv_version);
				int version;
				if (recv_version.val1_size()>0){
					version = stoi(recv_version.val1(0));
					versions[i] = version;
				}
				else{
					version = -1;
					versions[i] = -1; 
				}
				
				max_version = max(max_version, version);
				if (max_version==version){
					max_idx = i;
				}
				
				count+=1;				
			}	
		}

		if (count == fds.size()){
			break;
		}
		
		
	}


	request new_file;
	new_file.set_request_type("PUT");
	new_file.set_request_num(1);
	new_file.set_row(user);
	new_file.set_col(filename);
	new_file.add_val1(content);

	request new_version;
	new_version.set_request_type("PUT");
	new_version.set_request_num(0);
	new_version.set_row(user);
	//new_version.set_col("version/"+ pfolder + "/" + newname);
	new_version.set_col("version/"+ filename);
	int version = versions[max_idx]+1;
	if (version==0){
		version = 1;
	}
	new_version.add_val1(to_string(version));

		
	for (unsigned int idx=0; idx< fds.size(); idx++){					
		if (versions[idx]!=0){
			request req_recv1;
			sendmsg(fds[idx], new_file);
			readmsg(fds[idx], req_recv1);

			request req_recv2;
			sendmsg(fds[idx], new_version);
			readmsg(fds[idx], req_recv2);
		}		

		
		close(fds[idx]);
	}
	
	return true;
	
}

bool ask_pfolder_version(const vector<int>& fds, const string& row, const string& col, const vector<int>& file_versions, vector<int>& versions, int& max_idx){
					
	request req_version;
	req_version.set_request_type("GET");
	req_version.set_request_num(1);
	req_version.set_row(row);
	req_version.set_col("version/"+ col);
	//cout << "version/"+ col << endl;
	
	int max_version = -1;
	
	for (unsigned int i=0; i<fds.size(); i++){
		versions.push_back(0);
		if (file_versions[i]>0){			
			
			sendmsg(fds[i], req_version);
			
			request recv_version;
			readmsg(fds[i], recv_version);

			int version;
			if (recv_version.val1_size()>0){
				version = stoi(recv_version.val1(0));
				versions[i] = version;
			}
			else{
				version = -1;
				versions[i] = -1; 
			}
			
			
			max_version = max(max_version, version);
			if (max_version==version){
				max_idx = i;
			}			
		}
							
	}

	if (max_version== -1){
		close_fds(fds);
		return false;
	}

	return true;	
}


bool replicate_change_password(const string& user, const string& oldpass, const string& newpass, const vector<int>& fds, const vector<int>& versions, int max_idx){

	 
	request get_password;
	get_password.set_request_type("GET");
	get_password.set_request_num(1);
	get_password.set_row(user);
	get_password.set_col("PASSWORD");

	request req_recv;
	sendmsg(fds[max_idx], get_password);
	readmsg(fds[max_idx], req_recv);

	if (req_recv.val1(0).compare(oldpass)!=0){
		return false;
	}

	request update_password;
	update_password.set_request_type("PUT");
	update_password.set_request_num(1);
	update_password.set_row(user);
	update_password.set_col("PASSWORD");
	update_password.add_val1(newpass);

	
	request update_password_version;
	update_password_version.set_request_type("CPUT");
	update_password_version.set_request_num(0);
	update_password_version.set_row(user);
	update_password_version.set_col("version/PASSWORD");
	update_password_version.add_val2(to_string(versions[max_idx]+1));	
	
	
	for (unsigned int idx=0; idx< fds.size(); idx++){
		if (versions[idx]>0){
			request req_recv1;			
					
			sendmsg(fds[idx], update_password);
			readmsg(fds[idx], req_recv1);

			
			if (update_password_version.val1_size()>0){
				update_password_version.clear_val1();
			}
			update_password_version.add_val1(to_string(versions[idx]));
			request req_recv2;
			sendmsg(fds[idx], update_password_version);
			readmsg(fds[idx], req_recv2);
		}
		close(fds[idx]);
	}

	return true;
}


bool replicate_new_user(const vector<string>& replicas, const string& user, const string& password, int quorum){

	vector<int> fds;

	for (unsigned int i=0; i<replicas.size(); i++){
		string ip;
		int port;
		get_ip_port(replicas[i], ip, port);cout<<port<<endl;
		int sockfd = socket(PF_INET, SOCK_STREAM, 0);
		
		sockaddr_in serv_addr;
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = PF_INET;
		serv_addr.sin_port = htons(port);				
		inet_pton(PF_INET, ip.c_str(), &(serv_addr.sin_addr));
		if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==0){
			//FD_SET(sockfd, &rfds);
			fds.push_back(sockfd);					 				
		}
		else{
			fprintf(stderr, "%s\n", "Error in connecting to server");
		}					
	}

	if (fds.size()<quorum){
		return false; 
	}

	request add_user;
	add_user.set_request_type("PUT");
	add_user.set_request_num(1);
	add_user.set_row(user);
	add_user.set_col("PASSWORD");
	add_user.add_val1(password);

	request add_pass_version;
	add_pass_version.set_request_type("PUT");
	add_pass_version.set_request_num(1);
	add_pass_version.set_row(user);
	add_pass_version.set_col("version/PASSWORD");
	add_pass_version.add_val1("1");

	request add_mailbox;
	add_mailbox.set_request_type("PUT");
	add_mailbox.set_request_num(1);
	add_mailbox.set_row(user);
	add_mailbox.set_col("MAILS/");

	request add_mailbox_version;
	add_mailbox_version.set_request_type("PUT");
	add_mailbox_version.set_request_num(1);
	add_mailbox_version.set_row(user);
	add_mailbox_version.set_col("version/MAILS/");	
	add_mailbox_version.add_val1("1");

	request add_drive;
	add_drive.set_request_type("PUT");
	add_drive.set_request_num(1);
	add_drive.set_row(user);
	add_drive.set_col("/");

	request add_drive_version;
	add_drive_version.set_request_type("PUT");
	add_drive_version.set_request_num(1);
	add_drive_version.set_row(user);
	add_drive_version.set_col("version//");	
	add_drive_version.add_val1("1");	 

	request add_trash;
	add_trash.set_request_type("PUT");
	add_trash.set_request_num(1);
	add_trash.set_row(user);
	add_trash.set_col("TRASH/");

	request add_trash_version;
	add_trash_version.set_request_type("PUT");
	add_trash_version.set_request_num(1);
	add_trash_version.set_row(user);
	add_trash_version.set_col("version/TRASH/");
	add_trash_version.add_val1("1");

	request add_contact;
	add_contact.set_request_type("PUT");
	add_contact.set_request_num(1);
	add_contact.set_row(user);
	add_contact.set_col("CONTACTS");

	request add_contact_version;
	add_contact_version.set_request_type("PUT");
	add_contact_version.set_request_num(0);
	add_contact_version.set_row(user);
	add_contact_version.set_col("version/CONTACTS");
	add_contact_version.add_val1("1");

	for (unsigned int idx=0; idx< fds.size(); idx++){

		request req_recv1;
		sendmsg(fds[idx], add_user);
		readmsg(fds[idx], req_recv1);

		request req_recv2;
		sendmsg(fds[idx], add_pass_version);
		readmsg(fds[idx], req_recv2);

		request req_recv3;
		sendmsg(fds[idx], add_mailbox);
		readmsg(fds[idx], req_recv3);

		request req_recv4;
		sendmsg(fds[idx], add_mailbox_version);
		readmsg(fds[idx], req_recv4);

		request req_recv5;
		sendmsg(fds[idx], add_drive);
		readmsg(fds[idx], req_recv5);

		request req_recv6;
		sendmsg(fds[idx], add_drive_version);
		readmsg(fds[idx], req_recv6);
		
		request req_recv7;
		sendmsg(fds[idx], add_trash);
		readmsg(fds[idx], req_recv7);

		request req_recv8;
		sendmsg(fds[idx], add_trash_version);
		readmsg(fds[idx], req_recv8);

		request req_recv9;
		sendmsg(fds[idx], add_contact);
		readmsg(fds[idx], req_recv9);

		request req_recv10;
		sendmsg(fds[idx], add_contact_version);
		readmsg(fds[idx], req_recv10);
		
		close(fds[idx]);
	}

	return true;
}


void replicate_contact(const string& user, const string& pfolder, const string& filename, const vector<int>& fds, const vector<int>& versions, int max_idx){
	
	request get_contacts;
	get_contacts.set_request_type("GET");
	get_contacts.set_request_num(1);
	get_contacts.set_row(user);
	get_contacts.set_col(pfolder);

	request add_contact;
	add_contact.set_request_type("CPUT");
	add_contact.set_request_num(1);
	add_contact.set_row(user);
	add_contact.set_col(pfolder);

	request update_version;
	update_version.set_request_type("CPUT");
	update_version.set_request_num(0);
	update_version.set_row(user);
	update_version.set_col("version/"+pfolder);
	update_version.add_val2(to_string(versions[max_idx]+1));

	for (unsigned int idx=0; idx< fds.size(); idx++){					
		if (versions[idx]>0){

			request req_recv1;
			sendmsg(fds[idx], get_contacts);
			readmsg(fds[idx], req_recv1);

			if (add_contact.val1_size()>0){
				add_contact.clear_val1();
			}
			if (add_contact.val2_size()>0){
				add_contact.clear_val2();
			}

			for (int i=0; i< req_recv1.val1_size(); i++){
				add_contact.add_val1(req_recv1.val1(i));
				add_contact.add_val2(req_recv1.val1(i)); 	
			}

			add_contact.add_val2(filename);

			request req_recv2;
			sendmsg(fds[idx], add_contact);
			readmsg(fds[idx], req_recv2);

			if (update_version.val1_size()>0){
				update_version.clear_val1();
			}
			update_version.add_val1(to_string(versions[idx]));
			
			request req_recv3;			
					
			sendmsg(fds[idx], update_version);
			readmsg(fds[idx], req_recv3);
		
		}
		
		close(fds[idx]);
	}	

}
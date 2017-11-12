
#include "fault_tolerance.hh"


string sep = "<<SEP>>";
int sep_len = sep.length();

void checkpoint(){
	ofstream outfile ("checkpoints/checkpoint" + to_string(server_num) + ".txt");
	
	for (unordered_map<string,map<string,vector<string>>> :: iterator it_row = key_val_store.begin(); it_row != key_val_store.end(); it_row++){
	
		for (map<string,vector<string>> :: iterator it_col = (it_row->second).begin(); it_col != (it_row->second).end(); it_col++){
			
			int value_size = it_col->second.size();

			outfile << it_row->first;
			outfile << sep;
			outfile << it_col->first;
			outfile << sep;
			outfile << value_size;
			outfile << sep;

			for (vector<string>::iterator it = (it_col->second).begin(); it != (it_col->second).end(); it++){
				outfile << *it;
				outfile << sep;
			}
		}
	}

	outfile.close();

}


void recover_from_checkpoint(ifstream& recover){
	//char array[1000000];
	string text;
	//ifstream myfile ("checkpoints/checkpoint" + to_string(server_num) + ".txt");

	// if (recover.is_open()){
	// 	while ( getline (recover,line)){
	// 		text = text + line;
	// 	}
	// 	recover.close();
	// }
	stringstream strStream;
	strStream << recover.rdbuf();//read the file
	text = strStream.str();
	recover.close();

	// int i=0;
	// while(!feof(recover)){
	// 	//cout << "entered" << endl;
	// 	fscanf(recover,"%c", &array[i]);
	// 	i++;
	// }

	// fclose(recover);
	// text = string(array);

	int start_pos = 0;
	int sep_pos = 0;
	string row;
	string col;
	int value_size;
	vector<string> values;

	while(true){

		sep_pos = text.find(sep, start_pos);
		if (sep_pos== string::npos){
			break;
		}
		row = text.substr(start_pos, sep_pos - start_pos);
		//cout << row << endl;

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		col = text.substr(start_pos, sep_pos - start_pos);
		//cout << col << endl;

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		value_size = stoi(text.substr(start_pos, sep_pos - start_pos));
		//cout << value_size << endl;

		for (int i=0; i<value_size; i++){
			start_pos = sep_pos+sep_len;
			sep_pos = text.find(sep, start_pos);
			values.push_back(text.substr(start_pos, sep_pos - start_pos));
			//cout << values[i] << endl;
		}


		key_val_store[row][col] = values;
		start_pos = sep_pos+sep_len;
		values.clear();
	}
	
}

void log_request(const request& req, const string& file){
	
	if (req.request_type().compare("")==0 || req.request_type().compare("GET")==0 || req.request_type().compare("GET_COLS")==0 || req.request_type().compare("GET_ROWS")==0){
		return;
	}
	ofstream logfile (file, ofstream::app);

	logfile << req.request_type();
	//cout << req.request_type() << endl;
	logfile << sep;
	logfile << req.request_num();
	logfile << sep;

	if (req.has_row()){
		logfile << "1";
		logfile << sep;
		logfile << req.row();
		logfile << sep;
	}
	else{
		logfile << "0";
		logfile << sep;
	}

	if (req.has_col()){
		logfile << "1";
		logfile << sep;
		logfile << req.col();
		logfile << sep;
	}
	else{
		logfile << "0";
		logfile << sep;
	}

	if (req.has_success()){
		logfile << "1";
		logfile << sep;
		logfile << to_string(req.success());
		logfile << sep;
	}
	else{
		logfile << "0";
		logfile << sep;
	}

	logfile << req.val1_size();
	logfile << sep;

	for (int i=0; i< req.val1_size(); i++){
		logfile << req.val1(i);
		logfile << sep;
	}
	
	logfile << req.val2_size();
	logfile << sep;

	for (int i=0; i< req.val2_size(); i++){
		logfile << req.val2(i);
		logfile << sep;
	}
	logfile.close();

}


void recover_from_log(const string& file){
	//string line;
	ifstream recover(file);

	string text;
	
	stringstream strStream;
	strStream << recover.rdbuf();//read the file
	text = strStream.str();
	recover.close();

	
	int start_pos = 0;
	int sep_pos;
	
	int val1_size;
	int val2_size;
	int is_present;

	while(true){
		request req;

		sep_pos = text.find(sep, start_pos);
		if (sep_pos== string::npos){
			break;
		}
		req.set_request_type(text.substr(start_pos, sep_pos - start_pos));
		//cout << text.substr(start_pos, sep_pos - start_pos) << endl;

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		req.set_request_num(stoi(text.substr(start_pos, sep_pos - start_pos)));
		//cout << stoi(text.substr(start_pos, sep_pos - start_pos)) << endl;

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		is_present = stoi(text.substr(start_pos, sep_pos - start_pos));

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		
		if(is_present==1){			
			req.set_row(text.substr(start_pos, sep_pos - start_pos));
			//cout << text.substr(start_pos, sep_pos - start_pos) << endl;
			start_pos = sep_pos+sep_len;
			sep_pos = text.find(sep, start_pos);
			is_present = stoi(text.substr(start_pos, sep_pos - start_pos));			
		}
		else{
			is_present = stoi(text.substr(start_pos, sep_pos - start_pos));			
		}

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		
		if(is_present==1){			
			req.set_col(text.substr(start_pos, sep_pos - start_pos));
			//cout << text.substr(start_pos, sep_pos - start_pos) << endl;
			start_pos = sep_pos+sep_len;
			sep_pos = text.find(sep, start_pos);
			is_present = stoi(text.substr(start_pos, sep_pos - start_pos));			
		}
		else{
			is_present = stoi(text.substr(start_pos, sep_pos - start_pos));			
		}

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);
		
		if(is_present==1){			
			req.set_success(stoi(text.substr(start_pos, sep_pos - start_pos)));
			start_pos = sep_pos+sep_len;
			sep_pos = text.find(sep, start_pos);
			val1_size = stoi(text.substr(start_pos, sep_pos - start_pos));			
		}
		else{
			val1_size = stoi(text.substr(start_pos, sep_pos - start_pos));			
		}

		for (int i=0; i<val1_size; i++){
			start_pos = sep_pos+sep_len;
			sep_pos = text.find(sep, start_pos);
			req.add_val1(text.substr(start_pos, sep_pos - start_pos));
			//cout << text.substr(start_pos, sep_pos - start_pos) << endl;
		}

		start_pos = sep_pos+sep_len;
		sep_pos = text.find(sep, start_pos);

		val2_size = stoi(text.substr(start_pos, sep_pos - start_pos));

		for (int i=0; i<val2_size; i++){
			start_pos = sep_pos+sep_len;
			sep_pos = text.find(sep, start_pos);
			req.add_val2(text.substr(start_pos, sep_pos - start_pos));
		}

		request res1 = process_request(req);
		start_pos = sep_pos+sep_len;
	}
	recover.close();
}


bool updates_from_backends(){
	// For each user
	for (unordered_map<string,map<string,vector<string>>> :: iterator it_row = key_val_store.begin(); it_row != key_val_store.end(); it_row++){
		string user = it_row->first;

		// Get replicas from master
		request req_replicas;
		req_replicas.set_request_type("GET");
		req_replicas.set_request_num(0);
		req_replicas.set_row(user);
		req_replicas.set_col("BACKEND");

		request req_recv;

		bool b = send_recv(req_recv, req_replicas, mstr_srv_addr.ip, mstr_srv_addr.port);
		if (b==false){
			return false;
		}
		vector<string> replicas;
		for (int i=0; i< req_recv.val1_size(); i++){
			if (req_recv.val1(i).compare(my_addr.ip+":"+to_string(my_addr.port)) !=0){
				replicas.push_back(req_recv.val1(i));
			}
		}

		unordered_set<string> all_columns;
		string vers = "version/";
		// Get columns from the from replicas
		request req_cols;
		req_cols.set_request_type("GET_COLS");
		req_cols.set_request_num(0);
		req_cols.set_row(user);

		
		for (unsigned int i=0; i< replicas.size(); i++){
			string ip;
			int port;
			get_ip_port(replicas[i], ip, port);
			request req_recv2;
			bool b = send_recv(req_recv2, req_cols, ip, port);
			//cout << "Entered" << endl;
			for (int i=0; i<req_recv2.val1_size(); i++){
				if (req_recv2.val1(i).substr(0,8).compare(vers)!=0){ // all except versions
					all_columns.insert(req_recv2.val1(i));
				}
			}
		}

		for (map<string,vector<string>> :: iterator it_col = (it_row->second).begin(); it_col != (it_row->second).end(); it_col++){
			if (it_col->first.substr(0,8).compare(vers)!=0){ // all except versions
				all_columns.insert(it_col->first);
			}
		}		

		vector<int> fds;
		vector<int> versions;
		int max_idx;
		string col;

		for (unordered_set<string> :: iterator it = all_columns.begin(); it != all_columns.end(); it++){
			col = *it;

			bool v = ask_version(replicas, user, col, R, fds, versions, max_idx, false);
			
			if (v== false){
				return false;
			}
			
			int max_version = versions[max_idx];
			if (key_val_store[user].find("version/"+col)!= key_val_store[user].end()){
				if (max_version<= stoi(key_val_store[user]["version/"+col][0])){
					close_fds(fds);
					fds.clear();
					versions.clear();
					continue;
				}

			}
			request req_data;
			req_data.set_request_type("GET");
			req_data.set_request_num(0);
			req_data.set_row(user);
			req_data.set_col(col);

			request recv_data;

			replicate_read(req_data, recv_data, fds, max_idx);
			vector<string> data;
			for (int i=0; i<recv_data.val1_size(); i++){
				data.push_back(recv_data.val1(i));
			}

			key_val_store[user][col] = data;
			vector <string> temp{to_string(max_version)};
			key_val_store[user]["version/"+col] = temp;

			fds.clear();
			versions.clear();
		}
	}
	return true;	
}



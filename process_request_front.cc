#include "process_request_front.hh"

string render_all_files(int& fd, string& folder,string& username){
	string con;
	if(folder.empty())
		folder="/";
	clean_text(folder);
	//cout<<"getting folder contents of "<<folder<<endl;
	vector<string> file_seq = get_all_mails_files(username,folder);
	folder+="/";
	clean_text(folder);
	con += "<h3 style=\"margin-left:10px\">Folder: "+folder+"</h3><br>\r\n";
	if(file_seq.size()>1 || (file_seq.size()>0 && folder.compare("/")==0))
		con += "<form method=\"post\" action=\"/folder\"><input type=\"submit\" value=\"Delete\" style=\"margin-left:10px\"><br><br>\r\n";
	else
		con += "<p style=\"margin-left:10px;color:red\">Folder is empty or does not exist! Please check the path and try again</p>\r\n";
	for(int m=0;m<file_seq.size();m++){
		//cout<<file_seq[m]<<endl;
		if(file_seq[m].empty())
			continue;
		con += "<p style=\"margin-left:10px\"><input type=\"checkbox\" name=delete value=\""+folder+file_seq[m]+"\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\""+folder+file_seq[m]+"\" download=\""+file_seq[m]+"\">"+file_seq[m]+"</a></p>";
	}
	if(file_seq.size()>0)
		con += "</form>\r\n";
	return con;
}

string render_all_mails(int& fd, string& username,string folder){
	string con;
	vector<string> mail_seq = get_all_mails_files(username,folder);
	for(int m=0;m<mail_seq.size();m++){
		string mail_content = get_mail_file_content(username,folder+ mail_seq[m]);
		int new_line_pos = mail_content.find("\r\n");
		string sender = mail_content.substr(7,new_line_pos-1-7); // From: < >\r\n
		string subject = mail_content.substr(new_line_pos+11,mail_content.find("\r\n",new_line_pos+2)-new_line_pos-2-9);
		con += "<tr><td><input type=\"checkbox\" name=delete value=\""+mail_seq[m]+"\"></td><td>"+sender+"</td><td><a href=\""+folder+mail_seq[m]+"\">"+subject+"</td></a></tr>";
	}
	return con;
}

string render_all_contacts(int& fd, string& username){

	string con;
	vector<string> file_seq = get_all_mails_files(username,"CONTACTS");

	if(file_seq.size()>0)
		con += "<form method=\"post\" action=\"/composemail\"><input type=\"submit\" value=\"Send mail\" style=\"margin-left:10px\"><br><br>\r\n";
	else
		con += "<p style=\"margin-left:10px;color:red\">No contacts!</p>\r\n";

	for(int m=0;m<file_seq.size();m++){
		con += "<p style=\"margin-left:10px\"><input type=\"checkbox\" name=rcpt value=\""+file_seq[m]+"\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"+file_seq[m]+"</p>";
	}
	if(file_seq.size()>0)
	con += "</form>\r\n";
	return con;
}

bool is_folder_empty(string& folder,string& username){

	folder += "/";
	clean_text(folder);

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	bool b = ask_version(clients_cache[username],username,folder,R,fds,versions,max_idx);
	if (b==false){
		return true;	
	}

	request req_payload;
	request res_payload;

	req_payload.set_request_type("GET");
	req_payload.set_row(username);
	req_payload.set_col(folder);
	req_payload.set_request_num(0);

	replicate_read(req_payload, res_payload, fds, max_idx);

	//res_payload = send_and_receive(req_payload);
	//cout<<folder<<" size "<<res_payload.val1.size()<<endl;
	if(res_payload.val1_size()==0 || (res_payload.val1_size()==1 && res_payload.val1(0).compare("")==0))
		return true;
	return false;

}

void rename_file(string& file_name,string& src_path,string& new_name,string& username){

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	//cout << mail_seq << endl;
	bool b = ask_version(clients_cache[username],username,src_path + file_name,W,fds,versions,max_idx);
	if (b==false){
		return;	
	}
	bool b1 = replicate_rename(username, src_path, file_name, new_name, fds, versions, max_idx);	
}

void move_file(string& file_name,string src_path,string dst_path,string& username){

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	//cout << mail_seq << endl;
	bool b = ask_version(clients_cache[username],username,src_path + file_name,W,fds,versions,max_idx);
	if (b==false){
		return;	
	}
	bool b1 = replicate_move(username, src_path, dst_path, file_name, fds, versions, max_idx);
}


bool authenticated(string& username,string& password){

	vector <string> val2;

	if (clients_cache.find(username) == clients_cache.end()){
		request req_payload;
		req_payload.set_request_type("GET");
		req_payload.set_row(username);
		//req_payload.set_col("version/PASSWORD");
		req_payload.set_request_num(0);

		request req_recv;
		send_recv(req_recv, req_payload, master_back.ip, master_back.port);
		
		for (int i=0; i<req_recv.val2_size(); i++){
			val2.push_back(req_recv.val2(i));
		}
	}
	else{
		
	 	val2 = clients_cache[username];
	}

	vector<int> fds;
	int max_idx;
	vector<int> versions;

	bool b = ask_version(val2,username,"PASSWORD",R,fds,versions,max_idx);
	
	if (b==false){
		return false;
	}

	request req_pass;
	req_pass.set_request_type("GET");
	req_pass.set_row(username);
	req_pass.set_col("PASSWORD");
	req_pass.set_request_num(0);

	request recv_pass;
	replicate_read(req_pass, recv_pass, fds, max_idx);
	
				
	if(recv_pass.val1(0).compare(password)==0){
		clients_cache[username] = val2;
		return true;
	}

	return false;
}

bool add_new_user(string& username,string& password){

	request check_user;
	check_user.set_request_type("GET");
	check_user.set_row(username);
	check_user.set_request_num(0);

	//request_details res_payload = send_and_receive(check_user);
	request req_recv;
	send_recv(req_recv, check_user, master_back.ip, master_back.port);
	//cout<<req_recv.val1_size()<<endl;	
	
	vector <string> val2;
	for (int i=0; i<req_recv.val2_size(); i++){
		val2.push_back(req_recv.val2(i));
	}
	
	vector<int> fds;
	int max_idx;
	vector<int> versions;

	bool b1 = ask_version(val2,username,"PASSWORD",R,fds,versions,max_idx);

	
	if (b1==true){
		close_fds(fds);
		return false;
	}
	
	//clients_cache[username] = val2;

	bool b2 = replicate_new_user(val2, username, password, W);	
	
	return b2;
}

vector<string> get_all_mails_files(string& username,string col){

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	vector<string> folder_content;

	bool b = ask_version(clients_cache[username],username,col,R,fds,versions,max_idx);
	if (b==false){
		return folder_content;	
	}

	request req_payload,res_payload;
	req_payload.set_request_type("GET");
	req_payload.set_row(username);
	req_payload.set_col(col);
	req_payload.set_request_num(0);

	replicate_read(req_payload, res_payload, fds, max_idx);

	
	for(int i=0; i<res_payload.val1_size(); i++)
		folder_content.push_back(res_payload.val1(i));

	return folder_content;
}

string get_mail_file_content(string& username,string mail_seq){

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	bool b = ask_version(clients_cache[username],username,mail_seq,R,fds,versions,max_idx);
	if (b==false){
		return "";
	}

	request req_payload,res_payload;
	req_payload.set_request_type("GET");
	req_payload.set_row(username);
	req_payload.set_col(mail_seq);
	req_payload.set_request_num(0);

	replicate_read(req_payload, res_payload, fds, max_idx);

	if(res_payload.val1_size()>0)
		return res_payload.val1(0);
	return "";
}

void save_file(string& username,string folder,string& filename,string& file_data){

	folder += "/";
	clean_text(folder);

	bool b = replicate_upload(clients_cache[username], username, folder, filename, file_data,  W);
}

string get_session_id(string& username){


	if (clients_cache.find(username) == clients_cache.end()){
		request check_user;
		check_user.set_request_type("GET");
		check_user.set_row(username);
		check_user.set_request_num(0);

		request req_recv;
		send_recv(req_recv, check_user, master_back.ip, master_back.port);
		
		vector <string> val2;
		for (int i=0; i<req_recv.val2_size(); i++){
			val2.push_back(req_recv.val2(i));
		}
		clients_cache[username] = val2;
	}

	request req_payload;
	req_payload.set_request_type("GET");
	req_payload.set_row(username);
	req_payload.set_col("SID");
	req_payload.set_request_num(0);

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	bool b = ask_version(clients_cache[username],username,"SID",R,fds,versions,max_idx);
	if (!b){
		return "";
	}
	request res_payload;
	replicate_read(req_payload, res_payload, fds, max_idx);

	//request_details res_payload = send_and_receive(req_payload);
	if(res_payload.val1_size()>0)
		return res_payload.val1(0);
	return "";
	//return res_payload.val1[0];
}

void put_session_id(string& username, string& sid){
	
	replicate_put(clients_cache[username], username, "SID", sid, W);
	
}

void add_contact(string& username, string& contact){

	vector<int> fds;
	int max_idx;
	vector<int> versions;
	bool b = ask_version(clients_cache[username],username,"CONTACTS",W,fds,versions,max_idx);
	if (b==false){
		return;
	}	
	replicate_contact(username, "CONTACTS", contact, fds, versions, max_idx);
}

void delete_mail_file(string& username,string file_name,string col){

	//cout<<"Deleting "<<col+file_name<<endl;
	vector<int> fds;
	int max_idx;
	vector<int> versions;
	//cout << col << endl;
	bool b = ask_version(clients_cache[username],username,col+file_name, W, fds, versions, max_idx);
	if (b==false){
		return;
	}
	replicate_delete(username, col, file_name, fds, versions, max_idx);
}

void create_new_folder(string par_folder, string& new_folder, string& username){

	par_folder += "/";
	clean_text(par_folder);

	new_folder += "/";
	clean_text(new_folder);		

	bool b = replicate_upload(clients_cache[username], username, par_folder, new_folder, "",  W);
}

bool change_password(string& username,string& old_password,string& new_password){

        vector<int> fds;
        int max_idx;
        vector<int> versions;

        bool b = ask_version(clients_cache[username],username,"PASSWORD", W, fds, versions, max_idx);
        if (b==false){
                return false;
        }
        bool b2 = replicate_change_password(username, old_password, new_password, fds, versions, max_idx);
        return b2;

}

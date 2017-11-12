#include "server_front.hh"
#include "process_request_front.hh"

hash<string> str_hash;
int listen_fd;
bool verbose;
long long num_requests;
server_addr mstr_srv_addr,my_addr;
server_addr master_back;
unordered_map<string,string> sids;
unordered_map<string,vector<string>> clients_cache;



int main(int argc, char *argv[])
{

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

	int server_num = atoi(argv[optind + 1]);
	int lineNum = 0;
	while(getline(addr_file,line)){
		lineNum++;
		if(lineNum == server_num){
	        colon_pos = line.find(':');
	        my_addr.ip = line.substr(0,colon_pos);
	        my_addr.port = stoi(line.substr(colon_pos+1));
	        break;
	    }
	}

	master_back.ip = "127.0.0.1";
	master_back.port = 5001;
	num_requests = 0;

	listen_for_connections();

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
		
        pthread_t thread;
        pthread_create(&thread, &attr, worker, fd);
        all_threads.push_back(thread);
    }

    for(int i=0; i<all_threads.size(); i++){
        pthread_join(all_threads[i],NULL);
    }
}

void * worker(void *arg){

    int fd = *(int*)arg;

    num_requests ++;
    
    while(true){
    	string request;
    	char buf[100];
    	int rlen,content_len,end_pos;
    	string page_name,content;

    	while(true){
    		rlen = read(fd,&buf,100);
    		if(rlen==0)
    			break;
    		buf[rlen]='\0';
    		request += string(&buf[0],rlen);
    		if(request.find("\r\n\r\n") !=string::npos){
    			int start_pos = request.find(" ");
    			end_pos = request.find(" ",start_pos+1);
    			page_name = request.substr(start_pos+2,end_pos-start_pos-2);
    			start_pos = request.find("Content-Length");
    			end_pos = request.find("\r\n",start_pos+1);

    			if(page_name.compare("fileuploaded") == 0 || page_name.compare("mailcomposed") == 0
    					|| start_pos!=string::npos){
    				content_len = stoi(request.substr(start_pos+16,end_pos-start_pos-16));
    				int total_len = 0;
    				content = request.substr(end_pos+4);
    				total_len = content.size();
    				while(total_len<content_len){
    					rlen = read(fd,&buf,100);
    					if(rlen==0)
    					    break;
    					buf[rlen]='\0';
    					content += string(&buf[0],rlen);
    					total_len += rlen;
    				}
    			}
    			break;
    		}
    	}
    	cout<<page_name<<" "<<fd<<endl;
	if(rlen==0 || fcntl(fd, F_GETFD) == -1){
    		num_requests--;
    		close(fd);
    		break;
    	}
    	int cookie_pos = request.find("Cookie:");

    	string cookies[2];

    	if(cookie_pos != string::npos){
    		end_pos = request.find("\r\n",cookie_pos);
    		string t = request.substr(cookie_pos,end_pos-cookie_pos);
    		int user_pos = t.find("user");
    		int sid_pos = t.find("sid");
    		cookies[0] = t.substr(user_pos+5,sid_pos-user_pos-7);
    		cookies[1] = t.substr(sid_pos+4);
    	}

    	if(request.substr(0,4).compare("LOAD") == 0){
    		num_requests--;
		string load_val = to_string(num_requests) + "\r\n\r\n";
    		write(fd,load_val.c_str(),load_val.size());
		close(fd);
    		break;
    	}
    	else if(request.substr(0,4).compare("QUIT") == 0){
		num_requests--;
    		exit(0);
    	}
    	else if(page_name.substr(0,4).compare("favicon.ico") == 0){
    		 write(fd,"HTTP/1.1 204 No Content\r\n",17);
    		 write(fd,"Connection: keep-alive\r\n",24);
    		 write(fd,"Content-type: text/html; charset=UTF-8\r\n",40);
    		 write(fd,"Keep-Alive: timeout=15, max=100\r\n",33);
    		 write(fd,"\r\n",2);
    	}
    	else{
    	    write(fd,"HTTP/1.1 200 OK\r\n",17);
    	    write(fd,"Connection: Keep-Alive\r\n",24);
    	    write(fd,"Content-type: text/html; charset=UTF-8\r\n",40);
    	    write(fd,"Keep-Alive: timeout=25, max=100\r\n",33);

    		render_page(fd,page_name,request,content,cookies);

    	}
    }
}

void render_page(int fd,string page_name,string request,string content,string cookies[2]){

	FILE *fr;
	char *line = NULL;
    size_t len = 0;
    ssize_t readbytes;

    if(!cookies[1].empty() && sids[cookies[1]].empty()){
    	string t = get_session_id(cookies[0]);
    	if(cookies[1].compare(t) == 0){
    		sids[cookies[1]]= cookies[0];
    	}
    }

    string cookie = cookies[1];

	if(cookie.empty() || sids[cookie].empty()){
		if(page_name.compare("signup") == 0){
			int username_pos = content.find("username");
			if(username_pos == string::npos){
				render_simple_page(fd, "signup.html");
			}
			else{
				int password_pos = content.find("password");
				string username = content.substr(username_pos+9,password_pos-username_pos-10);
				string password = content.substr(password_pos+9);
				bool exists = add_new_user(username,password);

				int count = 0;
				string con;
				fr = fopen("signedup.html", "r");
				while((readbytes = getline(&line, &len, fr)) != -1 ){
					count++;
					if(count==21){
						if(!exists)
							con += "<h2>User already exists</h2>";
						else
							con += line;
					}
					else
						con += line;
				}
				string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
				write(fd,conlen.c_str(),conlen.size());
				write(fd,"\r\n",2);
				write(fd,con.c_str(),con.size());
				write(fd,"\r\n",2);

			}
		}
		else if(page_name.compare("images/background.jpeg") == 0){
			write(fd,"\r\n",2);
			fr = fopen(page_name.c_str(), "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				write(fd,line,readbytes);
			}
			//string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			//write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			//write(fd,con.c_str(),con.size());
			//write(fd,"\r\n",2);
		}
		else{
			int username_pos = content.find("username");
			if(username_pos == string::npos){
				render_simple_page(fd, "login.html");
			}
			else{
				int password_pos = content.find("password");
				string username = content.substr(username_pos+9,password_pos-username_pos-10);
				string password = content.substr(password_pos+9);

				if(authenticated(username,password)){
					string hashed_cookie = to_string(str_hash(username));
					sids.insert({hashed_cookie,username});

					string temp = "Set-Cookie: user="+username+"\r\n";
					write(fd,temp.c_str(),temp.size());
					temp = "Set-Cookie: sid="+hashed_cookie+"\r\n";
					write(fd,temp.c_str(),temp.size());
					write(fd,"\r\n",2);
					put_session_id(username,hashed_cookie);
					int count = 0;
					fr = fopen("myaccount.html", "r");
					while((readbytes = getline(&line, &len, fr)) != -1 ){
						count++;
						if(count==39){
						string temp = "&nbsp;Welcome " + username + "!";
							write(fd,temp.c_str(),temp.size());
							write(fd,"\r\n",2);
						}
						write(fd,line,readbytes);
						write(fd,"\r\n",2);
					}
				}
				else{
					int count = 0;
					fr = fopen("login.html", "r");
					string con;
					while((readbytes = getline(&line, &len, fr)) != -1 ){
						count++;
						if(count==55){
							con += "<h3 style=\"margin-left:150px\">Invalid username or password</h3><br>";
						}
						con += line;
					}
					string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
					write(fd,conlen.c_str(),conlen.size());
					write(fd,"\r\n",2);
					write(fd,con.c_str(),con.size());
					write(fd,"\r\n",2);
				}
			}
		}

	}
	else if(!cookie.empty()){
		string username = sids[cookie];
		if(page_name.empty() || page_name.compare("myaccount") == 0){
			int count = 0;
			string con;
			fr = fopen("myaccount.html", "r");
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==39){
					con += "&nbsp;Welcome " + username + "!";
				}
				con += line;
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("changepassword") == 0){
			render_simple_page(fd,"changepassword.html");
		}
		else if(page_name.compare("passchanged") == 0){
			int username_pos = content.find("username");
			int old_pass_pos = content.find("oldpassword");
			int new_pass_pos = content.find("newpassword");
			string username = content.substr(username_pos+9,old_pass_pos-username_pos-10);
			string old_password = content.substr(old_pass_pos+12,new_pass_pos-old_pass_pos-13);
			string new_password = content.substr(new_pass_pos+12);
			bool isChanged = change_password(username,old_password,new_password);
			int count = 0;
			fr = fopen("passchanged.html", "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==22){
					if(isChanged)
						con += "<h2>Password changed successfully!</h2>\r\n";
					else
						con += "<h2>Incorrect username or password</h2>\r\n";
				}
				con += line;
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("mails") == 0){
			int delete_pos=-1;
			while((delete_pos = content.find("delete", delete_pos+1)) != string::npos){
					int and_pos = content.find("&", delete_pos);
					string name = content.substr(delete_pos+7,and_pos-delete_pos-7);
					move_file(name,"MAILS/","TRASH/",username);
			}
			int count = 0;
			fr = fopen("mails.html", "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==38){
					con += render_all_mails(fd,username,"MAILS/");
				}
				else{
					con += line;
				}
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("trash") == 0){
			int delete_pos=content.find("button");
			int and_pos = content.find("&");
			if(delete_pos!=string::npos){
				string button_val = content.substr(delete_pos+7,and_pos-delete_pos-7);
				content = content.substr(and_pos);
				delete_pos=-1;

				if(button_val.compare("Delete") == 0){
					while((delete_pos = content.find("delete", delete_pos+1)) != string::npos){
						and_pos = content.find("&", delete_pos);
						string name = content.substr(delete_pos+7,and_pos-delete_pos-7);
						delete_mail_file(username,content.substr(delete_pos+7,and_pos-delete_pos-7),"TRASH/");
					}
				}
				else{
					while((delete_pos = content.find("delete", delete_pos+1)) != string::npos){
						and_pos = content.find("&", delete_pos);
						string name = content.substr(delete_pos+7,and_pos-delete_pos-7);
						move_file(name,"TRASH/","MAILS/",username);
					}
				}
			}
			int count = 0;
			fr = fopen("trash.html", "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==33){
					con += render_all_mails(fd,username,"TRASH/");
				}
				else{
					con += line;
				}
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("folder") ==0){
			int delete_pos=-1;
			clean_text(content);

			while((delete_pos = content.find("delete", delete_pos+1)) != string::npos){
				int and_pos = content.find("&", delete_pos);
				string name = content.substr(delete_pos+7,and_pos-delete_pos-7);
				string folder_name,file_name;

				if(name[name.size()-1] == '/'){
					name = name.substr(0,name.size()-1);
					int slash_pos = -1;
					while((slash_pos = name.find("/", slash_pos+1)) != string::npos){
						folder_name = name.substr(0,slash_pos+1);
					}
					file_name = name.substr(folder_name.size()) + "/";
				}
				else{
					int slash_pos = -1;
					while((slash_pos = name.find("/", slash_pos+1)) != string::npos){
						folder_name = name.substr(0,slash_pos+1);
					}
					file_name = name.substr(folder_name.size());
				}

				if(file_name.find(".")!=string::npos || is_folder_empty(name,username)){
					delete_mail_file(username,file_name,folder_name);
				}
			}

			int count = 0;
			fr = fopen("folder.html", "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==36){
					int pos = content.find("getfolder");
					if(pos != string::npos){
						string folder = content.substr(pos+10,content.find("\r\n")-pos-10);
						clean_text(folder);
						con += render_all_files(fd,folder,username);
					}
				}
				con += line;
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("addressbook") == 0){
			int count = 0;
			fr = fopen("addressbook.html", "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==37){
					int pos;
					if(content.find("newcontact")!=string::npos){
						pos = content.find("newcontact");
						string contact = content.substr(pos+11,content.find("\r\n")-pos-11);
						clean_text(contact);
						add_contact(username,contact);
					}
					con += render_all_contacts(fd,username);
				}
				con += line;
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("files") == 0){
			int pos;
			if(content.find("newfolder")!=string::npos){
				pos = content.find("parentfolder");
				string par_folder = content.substr(pos+13,content.find("&")-pos-13);
				clean_text(par_folder);
				pos = content.find("newfolder");
				string new_folder = content.substr(pos+10,content.find("\r\n")-pos-10);
				clean_text(new_folder);
				create_new_folder(par_folder, new_folder, username);
			}
			else if(content.find("newname")!=string::npos){
				pos = content.find("filename");
				string file_name = content.substr(pos+9,content.find("&")-pos-9);
				clean_text(file_name);
				pos = content.find("srcpath");
				string src_path = content.substr(pos+8,content.find("&",pos)-pos-8);
				clean_text(src_path);
				pos = content.find("newname");
				string new_name = content.substr(pos+8,content.find("\r\n")-pos-8);
				clean_text(new_name);
				rename_file(file_name,src_path,new_name,username);
			}
			else if(content.find("dstpath")!=string::npos){
				pos = content.find("filename");
				string file_name = content.substr(pos+9,content.find("&")-pos-9);
				clean_text(file_name);
				pos = content.find("srcpath");
				string src_path = content.substr(pos+8,content.find("&",pos)-pos-8);
				clean_text(src_path);
				pos = content.find("dstpath");
				string dst_path = content.substr(pos+8,content.find("\r\n")-pos-8);
				clean_text(dst_path);
				//cout<<"Moving "<<file_name<<" "<<src_path<<" "<<dst_path<<endl;
				move_file(file_name,src_path,dst_path,username);
			}
			render_simple_page(fd,"files.html");
		}
		else if(page_name.compare("fileuploaded") == 0){
			content = content.substr(2);
			int end_pos = content.find("\r\n");
			string boundary = content.substr(0,end_pos);
			end_pos = content.find("\r\n\r\n");
			content = content.substr(end_pos+4);
			end_pos = content.find("\r\n");
			string folder = content.substr(0,end_pos);
			clean_text(folder);
			end_pos = content.find("\r\n",end_pos+2);
			content = content.substr(end_pos+2);
			int filename_pos = content.find("filename");
			end_pos = content.find("\r\n",filename_pos);
			string filename = content.substr(filename_pos+10,end_pos-filename_pos-11);
			end_pos = content.find("\r\n\r\n");
			string file_data = content.substr(end_pos+4,content.find(boundary,end_pos)-end_pos-6);
			//cout<<"File received "<< filename<<" for "<<folder<<" size "<<file_data.size()<<" boundary "<<boundary<<endl;
			save_file(username,folder,filename,file_data);
			render_simple_page(fd, "fileuploaded.html");
		}
		else if(page_name.compare("composemail") == 0){
			string rcpt_val;
			int rcpt_pos=-1;
			while((rcpt_pos = content.find("rcpt", rcpt_pos+1)) != string::npos){
				int and_pos = content.find("&", rcpt_pos);
				rcpt_val += content.substr(rcpt_pos+5,and_pos-rcpt_pos-5);
				if(and_pos!=string::npos)
					rcpt_val += ";";
			}
			clean_text(rcpt_val);
			int count = 0;
			fr = fopen("composemail.html", "r");
			string con;
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				count++;
				if(count==52){
					con += "<input type=\"text\" name=\"rcpt\" value=\""+rcpt_val+"\">";
				}
				else{
					con += line;
				}
			}
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("mailcomposed") == 0){
			int rcpt_pos = content.find("rcpt=");
			int subj_pos = content.find("subj=");
			int body_pos = content.find("body=");
			string rcpt = content.substr(rcpt_pos+5,subj_pos-rcpt_pos-6);
			string subj = content.substr(subj_pos+5,body_pos-subj_pos-6);
			string body = content.substr(body_pos+5);
			clean_text(rcpt);
			clean_text(subj);
			clean_text(body);

            vector<string> receivers;
            int start_pos = 0,prev_pos=0;
            while((start_pos = rcpt.find(";",start_pos+1)) != std::string::npos){
            	receivers.push_back(rcpt.substr(prev_pos,start_pos-prev_pos));
            	prev_pos = start_pos+1;
            }
            receivers.push_back(rcpt.substr(prev_pos));

            start_pos = -1;
            while((start_pos = body.find("%0D", start_pos+1)) != std::string::npos)
            	body.replace(start_pos, 3, "\r");
            start_pos = -1;
            while((start_pos = body.find("%0A", start_pos+1)) != std::string::npos)
            	body.replace(start_pos, 3, "\n");

            clean_text(body);
            bool isSent = smtpclient(username+"@localhost", receivers, subj, body);
			render_simple_page(fd, "mailcomposed.html");
		}
		else if(page_name.substr(0,5).compare("MAILS") == 0 || page_name.substr(0,5).compare("TRASH") == 0){
			string mail_content = get_mail_file_content(username,page_name);
			int start_pos = -1;
			while((start_pos = mail_content.find("\r\n", start_pos+1)) != std::string::npos)
				mail_content.replace(start_pos, 2, "<br> ");
			string sender = mail_content.substr(0,mail_content.find("<br>")+4);
			mail_content = mail_content.substr(sender.size());
			string subj = mail_content.substr(0,mail_content.find("<br>")+4);
			mail_content = mail_content.substr(subj.size());
			sender.replace(sender.find("<"), 1, "");
			sender.replace(sender.find(">"), 1, "");
			string con = "<html><head><title>Mail</title><style>table{width: 50%;}tr:nth-child(even){background-color:#dddddd;}td{border: 1px solid #dddddd;}</style></head><body><br><br><br><br><table><tr><td>"+sender+"</td></tr><tr><td>"+subj+"</td></tr><tr><td>"+mail_content+"</td></tr></table></body</html>";
			string conlen = "Content-Length: " + to_string(con.size())+"\r\n";
			write(fd,conlen.c_str(),conlen.size());
			write(fd,"\r\n",2);
			write(fd,con.c_str(),con.size());
			write(fd,"\r\n",2);
		}
		else if(page_name.compare("images/mail.jpeg") == 0 || page_name.compare("images/drive.jpeg") == 0 || page_name.compare("images/background.jpeg") == 0){
			write(fd,"\r\n",2);
			fr = fopen(page_name.c_str(), "r");
			while((readbytes = getline(&line, &len, fr)) != -1 ){
				write(fd,line,readbytes);
			}
			write(fd,"\r\n",2);

			if(page_name.compare("images/background.jpeg") == 0){
				num_requests--;
				close(fd);
			}
		}
		else if(page_name.compare("loggedout")==0){
			string temp = "Set-Cookie: user=\r\n";
			write(fd,temp.c_str(),temp.size());
			temp = "Set-Cookie: sid=\r\n";
			write(fd,temp.c_str(),temp.size());

			render_simple_page(fd, "loggedout.html");
		}
		else{
			string file_content = get_mail_file_content(username,"/"+page_name);
			string temp = "Content-Length: " + to_string(file_content.size())+"\r\n";
			write(fd,temp.c_str(),temp.size());
			write(fd,"\r\n",2);
			write(fd,file_content.c_str(),file_content.size());
			write(fd,"\r\n",2);

		}
	}
	else{
		cerr<<"dont know what happened"<<endl;
	}

	//close(fd);
}

void render_simple_page(int& fd, string file_name){
	FILE *fr;
	char *line = NULL;
	size_t len = 0;
	ssize_t readbytes;
	fr = fopen(file_name.c_str(), "r");
	string temp;
	while((readbytes = getline(&line, &len, fr)) != -1 ){
		temp += line;
	}
	string conlen = "Content-Length: " + to_string(temp.size())+"\r\n";
	write(fd,conlen.c_str(),conlen.size());
	write(fd,"\r\n",2);
	write(fd,temp.c_str(),temp.size());
	write(fd,"\r\n",2);

}

request_details send_and_receive(request req_payload){

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(5001);
    int ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	//SEND REQUEST
	int siz = req_payload.ByteSize()+4;
	char *pkt = new char [siz];
	google::protobuf::io::ArrayOutputStream aos(pkt,siz);
	CodedOutputStream *coded_output = new CodedOutputStream(&aos);
	coded_output->WriteVarint32(req_payload.ByteSize());
	req_payload.SerializeToCodedStream(coded_output);
	//write(sockfd,pkt,sizeof(pkt));
	send(sockfd, (void *) pkt,siz,0);

	//RECEIVE RESPONSE
	char buffer[4];
	int bytecount=0;

	if((bytecount = recv(sockfd,buffer,4, MSG_PEEK))== -1){
		cerr<<"Error receiving data"<<endl;
	}
	
	request_details res_payload = readBody(sockfd,readHdr(buffer));
	
	close(sockfd);

	return res_payload;
}

void clean_text(string& text){
	int start_pos = 0;
	while((start_pos = text.find("+", start_pos)) != std::string::npos)
		text.replace(start_pos, 1, " ");
	start_pos = 0;
	while((start_pos = text.find("%40", start_pos)) != std::string::npos){
		text.replace(start_pos, 3, "@");
	}
	start_pos = 0;
	while((start_pos = text.find("%2F", start_pos)) != std::string::npos){
		text.replace(start_pos, 3, "/");
	}
	start_pos = 0;
	while((start_pos = text.find("//", start_pos)) != std::string::npos){
		text.replace(start_pos, 1, "");
	}
	start_pos = 0;
	while((start_pos = text.find("%3B", start_pos)) != std::string::npos){
		text.replace(start_pos, 3, ";");
	}
}

request_details readBody(int comm_fd,google::protobuf::uint32 siz){

	int bytecount;
	request payload;
	char buffer [siz+4];

	if((bytecount = recv(comm_fd, (void *)buffer, 4+siz, MSG_WAITALL))== -1){
		cerr<<"Error receiving data"<<endl;
	}

	google::protobuf::io::ArrayInputStream ais(buffer,siz+4);
	CodedInputStream coded_input(&ais);
	coded_input.ReadVarint32(&siz);
	google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(siz);

	payload.ParseFromCodedStream(&coded_input);

	coded_input.PopLimit(msgLimit);

	request_details details;

	details.req_type = payload.request_type();
	details.req_num = payload.request_num();
	details.success = payload.success();

	if(payload.has_row()) details.row = payload.row();
	if(payload.has_col()) details.row = payload.col();

	for(int i=0; i<payload.val1_size(); i++)
		details.val1.push_back(payload.val1(i));

	for(int i=0; i<payload.val2_size(); i++)
			details.val2.push_back(payload.val2(i));

	return details;
}

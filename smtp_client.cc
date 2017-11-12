#include "smtp_client.h"

int debug = 0;
string smtpserv_ip = "127.0.0.1";
int smtpserv_port = 2500;

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



char* parse_record (unsigned char *buffer, size_t r,
                   const char *section, ns_sect s,
                   int idx, ns_msg *m) {

    ns_rr rr;
    int k = ns_parserr (m, s, idx, &rr);
    if (k == -1) {
        std::cerr << errno << " " << strerror (errno) << "\n";
        return NULL;
    }
    char* output;
    
    const size_t size = NS_MAXDNAME;
    unsigned char name[size];
    int t = ns_rr_type (rr);
    const u_char *data = ns_rr_rdata (rr);
    if (t == T_MX) {
        int pref = ns_get16 (data);
        ns_name_unpack (buffer, buffer + r, data + sizeof (u_int16_t),
                        name, size);
        char name2[size];
        ns_name_ntop (name, name2, size);
        output = name2;
        
    }
    else if (t == T_A) {
        unsigned int addr = ns_get32 (data);
        struct in_addr in;
        in.s_addr = ntohl (addr);
        char *a = inet_ntoa (in);
        output = a;
    }
    else if (t == T_NS) {
        ns_name_unpack (buffer, buffer + r, data, name, size);
        char name2[size];
        ns_name_ntop (name, name2, size);
        output = name2;
    }
    else {
    	char temp[] = "unhandled record";
        output = temp;
    }

    return output;
}

void read_reply(int sockfd, string& reply){
	char* msg = new char [1000];
	int idx = 0;
	while(true){
		//cout << "read_enter" << endl;
		read(sockfd, &msg[idx], 1);
		if (msg[idx]=='\n'){
			msg[idx-1] = '\0';
			reply = string(msg);
			break;
		}
		else{
			idx++;
		}
	}
	delete[] msg; 
}


int sendmail(int sockfd, const string& sender, const string& receiver, const string& subject, const string& message){

	char crlf[] = "\r\n";
	int move_ahead = 0;
	string reply;
	size_t found = receiver.find("@");
	string domain = receiver.substr(found+1,receiver.length()-found-1);
	string from_line = "From: <" + sender + ">\r\n";
	string to_line = "To: <" + receiver + ">\r\n";
	string sub = "Subject: " + subject + "\r\n";
	string appended_message = from_line + sub + to_line + message;

	read_reply(sockfd, reply);
	if (debug==1){
		fprintf(stderr, "[%d] %s", sockfd, "S: ");
		fprintf(stderr, "%s\n", reply.c_str());							
	}

	if (reply.substr(0,3).compare("220")==0){
		move_ahead = 1;
	}
	else{
		move_ahead = 0;	
	}

	if (move_ahead==1){

		string cmnd = "Helo " + domain; 
		do_write(sockfd, cmnd.c_str(), cmnd.length());
		do_write(sockfd, crlf, 2);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "C: ");
			fprintf(stderr, "%s\n", cmnd.c_str());							
		}

		
		read_reply(sockfd, reply);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "S: ");
			fprintf(stderr, "%s\n", reply.c_str());							
		}

		if (reply.substr(0,3).compare("250")==0){
			move_ahead = 1;
		}
		else{
			move_ahead = 0;
		}
	}
	
	if (move_ahead == 1){
		string cmnd = "Mail From: <"+ sender + ">";
		do_write(sockfd, cmnd.c_str(), cmnd.length());
		do_write(sockfd, crlf, 2);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "C: ");
			fprintf(stderr, "%s\n", cmnd.c_str());							
		}

		read_reply(sockfd, reply);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "S: ");
			fprintf(stderr, "%s\n", reply.c_str());							
		}

		if (reply.substr(0,3).compare("250")==0){
			move_ahead = 1;
		}
		else{
			move_ahead = 0;	
		}
		
	}
	

	if (move_ahead == 1){
		string cmnd = "Rcpt To: <"+ receiver + ">";
		do_write(sockfd, cmnd.c_str(), cmnd.length());
		do_write(sockfd, crlf, 2);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "C: ");
			fprintf(stderr, "%s\n", cmnd.c_str());							
		}

		read_reply(sockfd, reply);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "S: ");
			fprintf(stderr, "%s\n", reply.c_str());							
		}

		if (reply.substr(0,3).compare("250")==0){
			move_ahead = 1;
		}
		else{
			move_ahead = 0;	
		}
	}


	if (move_ahead == 1){
		string cmnd = "Data";
		do_write(sockfd, cmnd.c_str(), cmnd.length());
		do_write(sockfd, crlf, 2);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "C: ");
			fprintf(stderr, "%s\n", cmnd.c_str());							
		}
		
		read_reply(sockfd, reply);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "S: ");
			fprintf(stderr, "%s\n", reply.c_str());							
		}

		if (reply.substr(0,3).compare("354")==0){
			move_ahead = 1;
		}
		else{
			move_ahead = 0;	
		}

	}

	

	if (move_ahead == 1){
		//string cmnd = message;
		do_write(sockfd, appended_message.c_str(), appended_message.length());
		do_write(sockfd, "\r\n.\r\n", 5);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "C: ");
			fprintf(stderr, "%s", appended_message.c_str());
			fprintf(stderr, "%s\n", "\r\n.\r");							
		}		

		read_reply(sockfd, reply);
		
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "S: ");
			fprintf(stderr, "%s\n", reply.c_str());							
		}

		if (reply.substr(0,3).compare("250")==0){
			move_ahead = 1;
		}
		else{
			move_ahead = 0;	
		}
	}	

	

	if (move_ahead == 1){
		string cmnd = "quit";
		do_write(sockfd, cmnd.c_str(), cmnd.length());
		do_write(sockfd, crlf, 3);
		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "C: ");
			fprintf(stderr, "%s\n", cmnd.c_str());							
		}
		//cout << "entered1" << endl;
		read_reply(sockfd, reply);
		//cout << "entered2" << endl;

		if (debug==1){
			fprintf(stderr, "[%d] %s", sockfd, "S: ");
			fprintf(stderr, "%s\n", reply.c_str());							
		}

		if (reply.substr(0,3).compare("221")==0){
			move_ahead = 1;
		}
		else{
			move_ahead = 0;	
		}
	}

	return move_ahead;	
	
}

bool smtpclient(const string& sender, vector<string>& receivers, const string& subject, const string& message){
	for (unsigned int i=0; i< receivers.size(); i++){
		//cout<<"Sending to "<<receivers[i]<<endl;
		string receiver = receivers[i];
		int sockfd = socket(PF_INET,SOCK_STREAM,0);
	    sockaddr_in server_addr;		    			
		bzero(&server_addr, sizeof(server_addr));

		size_t found = receiver.find("@");
		string domain = receiver.substr(found+1,receiver.length()-found-1);
		int connected = 0;
		//cout<<"Domain "<<domain<<endl;
		if (domain.compare("localhost")==0){

			server_addr.sin_family = PF_INET;
			server_addr.sin_port = htons(smtpserv_port);				
			inet_pton(PF_INET, smtpserv_ip.c_str(), &(server_addr.sin_addr));
			connected = 1+connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
		}

		else{

			unsigned char buffer[1000];
			int r = res_query(domain.c_str(), C_IN, T_MX, buffer, 1000);
			HEADER *hdr = reinterpret_cast<HEADER*> (buffer);	
		    
		    int answers = ntohs (hdr->ancount);   			
	      	ns_msg m;
			int k = ns_initparse (buffer, r, &m);
			string fullname;


			for (int i = 0; i < answers; ++i) {
		        string record = string(parse_record (buffer, r, "answers", ns_s_an, i, &m));
		        if (record.compare("unhandled record")!=0){
		        	fullname = record;
		        	break;
		        }
		    }

		    if (fullname.length()>0){			   
			    struct hostent *he = gethostbyname(fullname.c_str());
    			struct in_addr **addr_list;
    			addr_list = (struct in_addr **)he->h_addr_list;   			
	
				
    			server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_list[0]));
				server_addr.sin_family = PF_INET;
				server_addr.sin_port = htons(25);			    
    			//cout << inet_addr(inet_ntoa(*addr_list[0])) << endl;
			    //inet_pton(PF_INET, fullname.c_str(), &(server_addr.sin_addr));

			    connected = 1+connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
			}
			else{
				connected = 0;
			}

		}

		int success;
		if (connected==1){
			success = sendmail(sockfd, sender, receiver, subject, message);
		}

		close(sockfd);

		/*if (connected==0 || success == 0){
			if (debug==1){
				fprintf(stderr, "%s\n", "Connection Error: Mail Could not be sent");
			}
			return false;
		}
		else{
			return true;
		}*/
		
	}
	return true;
}


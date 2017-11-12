#include "protobuf.h"

int readmsg(int fd,  request& payload){
	char buffer1[4];
    int bytecount=0;
    //string output,pl;
    //request payload;

    memset(buffer1,'\0', 4);
    
    //Peek into the socket and get the packet size
    if((bytecount = recv(fd, buffer1, 4, MSG_PEEK))== -1){
        return -1;
    }
    if (bytecount==0){
    	return 0;
    }

    google::protobuf::uint32 siz = readHdr(buffer1);    
	
	char buffer2[siz+4];//size of the payload and hdr
	//Read the entire buffer including the hdr
	if((bytecount = recv(fd, (void *)buffer2, 4+siz, MSG_WAITALL))== -1){
	    return -1;
	}
	if (bytecount==0){
    	return 0;
    }
	//cout << "read = " << bytecount << endl;
	//cout<< "Second read byte count is " << bytecount << endl;
	//Assign ArrayInputStream with enough memory 
	ArrayInputStream ais(buffer2,siz+4);
	CodedInputStream coded_input(&ais);
	//Read an unsigned integer with Varint encoding, truncating to 32 bits.
	coded_input.ReadVarint32(&siz);
	//After the message's length is read, PushLimit() is used to prevent the CodedInputStream 
	//from reading beyond that length.Limits are used when parsing length-delimited 
	//embedded messages
	CodedInputStream::Limit msgLimit = coded_input.PushLimit(siz);
	//De-Serialize
	payload.ParseFromCodedStream(&coded_input);
	//Once the embedded message has been parsed, PopLimit() is called to undo the limit
	coded_input.PopLimit(msgLimit);
	return bytecount;

}

int sendmsg(int fd, const request& payload){
	 //cout << "entered sent " << endl;
	// cout << payload.request_type() << endl;
	// cout << payload.request_num() << endl;
	// cout << payload.row() << endl;
	// cout << payload.col() << endl;

	int bytecount;
	int siz = payload.ByteSize()+4;
	char *pkt = new char [siz];
	ArrayOutputStream aos(pkt,siz);
	CodedOutputStream *coded_output = new CodedOutputStream(&aos);
	coded_output->WriteVarint32(payload.ByteSize());
	//cout << "err1" << endl;

	payload.SerializeToCodedStream(coded_output);
	//cout << "err2" << endl;
	if( (bytecount=send(fd, (void *) pkt,siz,0))== -1 ) {
        fprintf(stderr, "Error sending data %d\n", errno);        
    }
    return bytecount;
    //bytecount=send(fd, (void *) pkt,siz,0);
    //cout << "sent " << bytecount << endl;
}


google::protobuf::uint32 readHdr(char *buf)
{
	google::protobuf::uint32 size;
	ArrayInputStream ais(buf,4);
	CodedInputStream coded_input(&ais);
	coded_input.ReadVarint32(&size);//Decode the HDR and get the size
	//cout<<"size of payload is "<<size<<endl;
	return size;
}


bool send_recv(request& req_recv, const request& req_send, string ip, int port){
	int sfd = socket(PF_INET,SOCK_STREAM,0);
	sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = PF_INET;
	serv_addr.sin_port = htons(port);				
	inet_pton(PF_INET, ip.c_str(), &(serv_addr.sin_addr));
	if (connect(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
		fprintf(stderr, "%s\n", "Error in connecting to server");
		return false;
		//exit(1);
	}
	if ((sendmsg(sfd, req_send))==-1){
		close (sfd);
		return false;
	}	
	if ((readmsg(sfd, req_recv))<=0){
		close (sfd);
		return false;
	}	
	close (sfd);
	return true;
}

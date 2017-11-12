#include "process_req_bck.hh"

request process_request(const request& req_payload){
	
	if(req_payload.request_type().compare("GET") == 0)
		return process_get_req(req_payload);
	else if(req_payload.request_type().compare("PUT") == 0)
		return process_put_req(req_payload);
	else if(req_payload.request_type().compare("DELETE") == 0)
		return process_delete_req(req_payload);
	else if(req_payload.request_type().compare("CPUT") == 0)
		return process_cput_req(req_payload);
	else if(req_payload.request_type().compare("GET_ROWS") == 0)
		return process_get_rows_req(req_payload);
	else if(req_payload.request_type().compare("GET_COLS") == 0)
		return process_get_cols_req(req_payload);
	else if(req_payload.request_type().compare("QUIT") == 0){
		exit(0);
	}
        else{
                request req;
                req.set_request_type("GET");
                req.set_request_num(0);
                return req;
	}
}

request process_get_req(const request& req_payload){

	request res_payload;
	res_payload.set_request_type(req_payload.request_type());
	res_payload.set_request_num(req_payload.request_num());
	res_payload.set_row(req_payload.row());
	res_payload.set_col(req_payload.col());
	if (req_payload.has_fd()){
		res_payload.set_fd(req_payload.fd());
	}
	vector<string> data_read = read_elem(req_payload.row(),req_payload.col());
	for (unsigned int i=0; i<data_read.size(); i++){
		res_payload.add_val1(data_read[i]);
	}
	res_payload.set_success(true);

        //cout<<res_payload.row<<endl;
        //cout<<res_payload.col<<endl;

	return res_payload;
}

request process_put_req(const request& req_payload){
	request res_payload;
	res_payload.set_request_type(req_payload.request_type());
	res_payload.set_request_num(req_payload.request_num());
	res_payload.set_row(req_payload.row());
	res_payload.set_col(req_payload.col());
	
	if (req_payload.has_fd()){
		res_payload.set_fd(req_payload.fd());
	}
	vector<string> val1;
	for (int i=0; i< req_payload.val1_size(); i++){
		val1.push_back(req_payload.val1(i));
	}
	res_payload.set_success(insert_elem(req_payload.row(),req_payload.col(),val1));
	
	return res_payload;

}

request process_delete_req(const request& req_payload){

	request res_payload;
	res_payload.set_request_type(req_payload.request_type());
	res_payload.set_request_num(req_payload.request_num());
	res_payload.set_row(req_payload.row());
	res_payload.set_col(req_payload.col());
	res_payload.set_success(delete_elem(req_payload.row(),req_payload.col()));

	return res_payload;
}

request process_cput_req(const request& req_payload){
	request res_payload;
	res_payload.set_request_type(req_payload.request_type());
	res_payload.set_request_num(req_payload.request_num());
	res_payload.set_row(req_payload.row());
	res_payload.set_col(req_payload.col());

	vector <string> val1;
	vector <string> val2;
	for (int i=0; i<req_payload.val1_size(); i++){
		val1.push_back(req_payload.val1(i));
	}
	for (int i=0; i<req_payload.val2_size(); i++){
		val2.push_back(req_payload.val2(i));
	}
	//cout << "Entered2" << endl;
	res_payload.set_success(replace_elem(req_payload.row(),req_payload.col(),val1,val2));

	return res_payload;
}

request process_get_rows_req(const request& req_payload){

	request res_payload;
	res_payload.set_request_type(req_payload.request_type());
	res_payload.set_request_num(req_payload.request_num());
	vector<string> data_read = get_all_rows();
	for (unsigned int i=0; i<data_read.size(); i++){
		res_payload.add_val1(data_read[i]);
	}
	res_payload.set_success(true);

	return res_payload;
}

request process_get_cols_req(const request& req_payload){

	request res_payload;
	res_payload.set_request_type(req_payload.request_type());
	res_payload.set_request_num(req_payload.request_num());
	res_payload.set_row(req_payload.row());

	vector<string> data_read = get_all_cols(req_payload.row());
	for (unsigned int i=0; i<data_read.size(); i++){
		res_payload.add_val1(data_read[i]);
	}
	res_payload.set_success(true);

	return res_payload;
}

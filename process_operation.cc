#include "process_operation.hh"

bool insert_elem(const string& row, const string& col, const vector<string>& cell){

	// vector<string> new_val;
	// new_val.push_back(cell);	
	key_val_store[row][col] = cell;
	return true;
	
	// unordered_map<string,map<string,vector<string>>>::iterator it_row = key_val_store.find(row);

	// if(it_row == key_val_store.end()){
	// 	map<string,vector<string>> new_map;
	// 	vector<string> new_val;
	// 	new_val.push_back(cell);
	// 	new_map.insert({col,new_val});
	// 	key_val_store.insert({row,new_map});
	// }
	// else{
	// 	//cout << "entered1" << endl;
	// 	map<string,vector<string>>::iterator it_col = (it_row->second).find(col);

	// 	if(it_col == it_row->second.end()){
	// 		vector<string> new_val;
	// 		new_val.push_back(cell);
	// 		(it_row->second).insert({col,new_val});
	// 		//cout << "entered2" << endl;
	// 	}
	// 	else{
	// 		vector<string> new_val;
	// 		new_val.push_back(cell);
	// 		(it_row->second).insert({col,new_val});
	// 		//it_col->second.push_back(cell);
	// 	}
	// }
	// return true;	
}

bool delete_elem(const string& row, const string& col){

	unordered_map<string,map<string,vector<string>>>::iterator it_row = key_val_store.find(row);

	if(it_row == key_val_store.end())
		return false;

	map<string,vector<string>>::iterator it_col = (it_row->second).find(col);

	if(it_col == it_row->second.end())
		return false;

	it_row->second.erase(col);

	return true;

}


vector<string> read_elem(const string& row, const string& col){

	vector<string> res;

	unordered_map<string,map<string,vector<string>>>::iterator it_row = key_val_store.find(row);

	if(it_row == key_val_store.end())
		return res;

	map<string,vector<string>>::iterator it_col = (it_row->second).find(col);

	if(it_col == it_row->second.end())
		return res;

	return it_col->second;

}

bool replace_elem(const string& row, const string& col, const vector<string>& val1, const vector<string>& val2){

	//cout << "entered1" << endl;
	unordered_map<string,map<string,vector<string>>>::iterator it_row = key_val_store.find(row);
	
	if(it_row == key_val_store.end())
		return false;

	map<string,vector<string>>::iterator it_col = (it_row->second).find(col);

	if(it_col == it_row->second.end())
		return false;

	if (val1.size()!=it_col->second.size()){
		//cout << "entered4" << endl;
		return false;
	}

	unordered_set<string> val1_set(val1.begin(), val1.end());


	for(vector<string>::iterator it = it_col->second.begin();it != it_col->second.end();it++){
		val1_set.insert(*it);
		
	}
	if (val1_set.size()!=val1.size()){
		//cout << "entered3" << endl;
		return false;
	}

	//cout << "entered2" << endl;

	key_val_store[row][col] = val2;

	return true;

}

vector<string> get_all_rows(){
	vector<string> rows;
	for(unordered_map<string,map<string,vector<string>>>::iterator it = key_val_store.begin(); it!= key_val_store.end();it++){
		rows.push_back(it->first);
	}
	return rows;
}

vector<string> get_all_cols(const string& row){
	unordered_map<string,map<string,vector<string>>>::iterator it_row = key_val_store.find(row);
	vector<string> cols;
	for(map<string,vector<string>>::iterator it = it_row->second.begin(); it!=it_row->second.end();it++){
		cols.push_back(it->first);
	}
	return cols;
}


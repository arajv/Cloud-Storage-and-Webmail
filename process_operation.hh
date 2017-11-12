#ifndef _PROCESSOPRBCKH_
#define _PROCESSOPRBCKH_

#include "server_bck.hh"

//PUT an element in the map
bool insert_elem(const string& row, const string& col, const vector<string>& cell);

//Delete an element from the map
bool delete_elem(const string& row, const string& col);

//Read an element from the map
vector<string> read_elem(const string& row, const string& col);

//change the value of an element in the map
bool replace_elem(const string& row, const string& col, const vector<string>& val1, const vector<string>& val2);

//Get all rows in the maps
vector<string> get_all_rows();

//Get all columns for a row
vector<string> get_all_cols(const string& row);

#endif

#pragma once

#include "protobuf.h"
#include "utils.h"

// Ask versions of a given column to the replicas of the user
bool ask_version(const vector<string>& replicas, const string& row, const string& col, int quorum, vector<int>& fds, vector<int>& versions, int& max_idx, bool is_frontend = true);

// Ask the server (which has the highest version number) for the data 
void replicate_read(const request& req, request& req_recv, const vector<int>& fds, int max_idx);

// Delete the data from the servers forming the quorum
void replicate_delete(const string& user, const string& pfolder, const string& filename, const vector<int>& fds, const vector<int>& versions, int max_idx);

// Rename the file on the servers forming the quorum 
bool replicate_rename(const string& user, const string& pfolder, const string& filename, const string& newname, const vector<int>& fds, const vector<int>& versions, int max_idx);

// Move a file from a source folder to a target folder on the servers forming quorum
bool replicate_move(const string& user, const string& pfolder1, const string& pfolder2, const string& filename, const vector<int>& fds, const vector<int>& versions, int max_idx);

// Upload a file in a folder on the servers forming quorum
bool replicate_upload(const vector<string>& replicas, const string& user, const string& pfolder, const string& filename, const string& content,  int quorum, bool mail = false);

// Adding Session ID of the user on the servers forming quorum
bool replicate_put(const vector<string> replicas, const string& user, const string& filename, const string& content, int quorum);

// Ask versions of the parent folder while modifying any content inside the folder
bool ask_pfolder_version(const vector<int>& fds, const string& row, const string& col, const vector<int>& file_versions, vector<int>& versions, int& max_idx);

// Modifying the password on the servers forming quorum
bool replicate_change_password(const string& user, const string& oldpass, const string& newpass, const vector<int>& fds, const vector<int>& versions, int max_idx);

// Adding a new user account on the servers forming quorum
bool replicate_new_user(const vector<string>& replicas, const string& user, const string& password, int quorum);

// Adding email contacts in the address book of a user on the servers forming quorum 
void replicate_contact(const string& user, const string& pfolder, const string& filename, const vector<int>& fds, const vector<int>& versions, int max_idx);

#include "server_front.hh"
#include "replication.h"


/*Check if user name and password is correct
 * Set cookies if correct
 */
bool authenticated(string& username,string& password);

//Add user on sign up and assign space on drive
bool add_new_user(string& username,string& password);

//Upload file to appropriate folder
void save_file(string& username,string folder,string& filename,string& file_data);

//Retrieve all mails/files/contacts
vector<string> get_all_mails_files(string& username,string col);

//Retreive mail/file content
string get_mail_file_content(string& username,string mail_seq);

//Save session id
void put_session_id(string& username, string& sid);

//Retrieve session id
string get_session_id(string& username);

//Delete mail/file
void delete_mail_file(string& username,string file_name,string col);

//Render files inside a folder on the webpage
string render_all_files(int& fd, string& folder, string& username);

//Render all mails in inbox/trash on the webpage
string render_all_mails(int& fd, string& username, string folder);

//Create a new folder
void create_new_folder(string par_folder, string& new_folder, string& username);

//check if the folder is empty
bool is_folder_empty(string& folder,string& username);

//Rename file
void rename_file(string& file_name,string& src_path,string& new_name,string& username);

//Move file from one folder to another
void move_file(string& file_name,string src_path,string dst_path,string& username);

//change password
bool change_password(string& username,string& old_password,string& new_password);

//Render all contacts on the webpage
string render_all_contacts(int& fd, string& username);

//Add a new contact to the addressbook
void add_contact(string& username, string& contact);

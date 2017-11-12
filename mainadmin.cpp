#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <tuple>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "request.pb.h"
#include "protobuf.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace std;

bool v = false;
int adminPort;
int all_threads[100];
string adminDefaultIp = "127.0.0.1:";
vector<tuple<string, int>> backServers;
vector<tuple<string, int>> frontServers;
// sending proto message to server
void sendmsgProto(int fd, const request& payload){
	// cout << "entered sent " << endl;
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
	if( (bytecount=send(fd, (void *) pkt,siz, MSG_NOSIGNAL))== -1 ) 
	{
        fprintf(stderr, "Error sending data %d\n", errno);        
    }

    //bytecount=send(fd, (void *) pkt,siz,0);
}



int sendData(int sckt, char *pdata, int datalen)
{
    //unsigned char *pdata = (unsigned char *) data;
    int numSent;

    // send() can send fewer bytes than requested,
    // so call it in a loop until the specified data
    // has been sent in full...

    while (datalen > 0) {
      numSent = send(sckt, pdata, datalen, 0);
      if (numSent == -1) return -1;
      pdata += numSent;
      datalen -= numSent;
    }

    return 0;
}

// send to handle when server not running
int sendDataNoReply(int sckt, char *pdata, int datalen)
{
    //unsigned char *pdata = (unsigned char *) data;
    int numSent;

    // send() can send fewer bytes than requested,
    // so call it in a loop until the specified data
    // has been sent in full...

    while (datalen > 0) 
    {
      numSent = send(sckt, pdata, datalen, MSG_NOSIGNAL);
      if (numSent == -1) return -1;
      pdata += numSent;
      datalen -= numSent;
    }

    return 0;
}

// function to render html page for the first page of admin
string renderMainTable(vector<string> frontServers, vector<string> status1, vector<string> backServers,vector<string> status2)
{
		string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
				 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"
				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">To view the Various Front and Backend Servers</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Front End Servers</h1>\n";

   //  " <h5 class=\"w3-padding-32\">Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</h5>\n"
    //  "<p class=\"w3-text-grey\">Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Excepteur sint\n"
     //  " occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco\n"
      // " laboris nisi ut aliquip ex ea commodo consequat.</p>\n"
	string firstTableStart = "<table>\n<col width = \"150\"><col width = \"150\"><col width = \"150\"><tr>\n<th>server number</th>\n<th>server</th>\n<th>status</th>\n</tr>\n";
					//header = header + tableStart;
					string firstTableBody = "";
					for(int i = 0; i< status1.size(); i++)
					{
						string formOpen = "<tr>\n<form action =\"/getLoad\" method = \"POST\"> ";
						string serverNum = "<td><input type=\"text\" name=\"serverSelected\" value ="+to_string(i)+" readonly></td>\n";
						string serverName = "<td><p>"+frontServers[i]+"</p></td>\n";
						string serverStatus = "<td>"+status1[i]+"</td></form></tr>\n";
						//string serverSelect = "<td><input type = \"submit\" value = \"get details\">\n </td>\n</form></tr>\n" ;

						string total = formOpen + serverNum + serverName +serverStatus ;//+ serverSelect;
						firstTableBody = firstTableBody + total;

					}

					string FirstTableEnd="</table></div>\n"
										"</div>\n";

				string secondTable="<div class=\"w3-row-padding w3-light-grey w3-padding-64 w3-container\">\n"
 " <div class=\"w3-content\">\n"
  "    <h1>BackEnd Servers</h1>\n";
string secondTableStart = "</table>\n<table>\n<col width = \"150\"><col width = \"150\"><col width = \"150\"><tr>\n"
		"<th>server number</th>\n<th>server</th>\n<th>status</th>\n</tr>\n";
					string secondTableBody = "";
					for(int i = 0; i< status2.size(); i++)
					{
						string formOpen = "<tr>\n<form action =\"/getLoadBack\" method = \"POST\"> ";
						string serverNum = "<td><input type=\"text\" name=\"serverSelected\" value ="+to_string(i)+" readonly></td>\n";
						string serverName = "<td><p>"+backServers[i]+"</p></td>\n";						
						string serverStatus = "<td>"+status2[i]+"</td>";
						string serverSelect = "<td><input type = \"submit\" value = \"get details\">\n </td>\n</form></tr>\n" ;

						string total = formOpen + serverNum + serverName+serverStatus + serverSelect;
						secondTableBody = secondTableBody + total;

					}
  // "   <h5 class=\"w3-padding-32\">Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</h5>\n"
  //
	//			string trial ="   <p class=\"w3-text-grey\">Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Excepteur sint\n";
  // "     occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco\n"
   // "    laboris nisi ut aliquip ex ea commodo consequat.</p>\n"

			string end=" </table></div>\n"
"</div>\n"
"</body>\n"
"</html>\n";

		return header+firstTableStart + firstTableBody+FirstTableEnd+secondTable+secondTableStart + secondTableBody+end;
}

// function to render html page of server shutdown page
string renderQuitPage(vector<string> frontServers, vector<string> status1, vector<string> backServers,vector<string> status2)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
				 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"
				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">To Shutdown Various Front and Backend Servers</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Front End Servers</h1>\n";

	"</head>\n<body>\n<h1>Admin</h1>\n<h2>Front end server details</h2>\n";
string firstTableStart = "<table>\n<col width = \"150\"><col width = \"150\"><col width = \"150\"><tr>\n<th>server number</th>\n<th>server</th>\n<th>status</th>\n</tr>\n";
//header = header + tableStart;
string firstTableBody = "";
for(int i = 0; i< status1.size(); i++)
{
	string formOpen = "<tr>\n<form action =\"/quitFrontEndServer\" method = \"POST\"> ";
	string serverNum = "<td><input type=\"text\" name=\"serverSelected\" value ="+to_string(i)+" readonly></td>\n";
	string serverName = "<td><p>"+frontServers[i]+"</p></td>\n";
	string serverStatus = "<td>"+status1[i]+"</td>";
	string serverSelect = "<td><input type = \"submit\" value = \"shutdown\">\n </td>\n</form></tr>\n" ;

	string total = formOpen + serverNum + serverName+serverStatus + serverSelect;
	firstTableBody = firstTableBody + total;

}
					string FirstTableEnd="</table></div>\n"
										"</div>\n";

				string secondTable="<div class=\"w3-row-padding w3-light-grey w3-padding-64 w3-container\">\n"
 " <div class=\"w3-content\">\n"
  "    <h1>BackEnd Servers</h1>\n";


				string secondTableStart = "</table>\n<h2>Back end server detials</h2>\n<table>\n<col width = \"150\"><col width = \"150\"><col width = \"150\"><tr>\n"
						"<th>server number</th>\n<th>server</th>\n<th>status</th>\n</tr>\n";
					string secondTableBody = "";
					for(int i = 0; i< status2.size(); i++)
					{
						string formOpen = "<tr>\n<form action =\"/quitBackEndServer\" method = \"POST\"> ";
						string serverNum = "<td><input type=\"text\" name=\"serverSelected\" value ="+to_string(i)+" readonly></td>\n";
						string serverName = "<td><p>"+backServers[i]+"</p></td>\n";
						string serverStatus = "<td>"+status2[i]+"</td>";
						string serverSelect = "<td><input type = \"submit\" value = \"shutdown\">\n </td>\n</form></tr>\n" ;

						string total = formOpen + serverNum + serverName+serverStatus + serverSelect;
						secondTableBody = secondTableBody + total;

					}
			string end=" </table></div>\n"
"</div>\n"
"</body>\n"
"</html>\n";

		return header+firstTableStart + firstTableBody+FirstTableEnd+secondTable+secondTableStart + secondTableBody+end;
}

// function to render html page of the start server page
string renderStartPage(vector<string> frontServers, vector<string> status1, vector<string> backServers,vector<string> status2)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
				 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"

				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">To Start Various Front and Backend Servers</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Front End Servers</h1>\n";

	"</head>\n<body>\n<h1>Admin</h1>\n<h2>Front end server details</h2>\n";
string firstTableStart = "<table>\n<col width = \"150\"><col width = \"150\"><col width = \"150\"><tr>\n<th>server number</th>\n<th>server</th>\n<th>status</th>\n</tr>\n";
//header = header + tableStart;
string firstTableBody = "";
for(int i = 0; i< status1.size(); i++)
{
	string formOpen = "<tr>\n<form action =\"/startFrontEndServer\" method = \"POST\"> ";
	string serverNum = "<td><input type=\"text\" name=\"serverSelected\" value ="+to_string(i)+" readonly></td>\n";
	string serverName = "<td><p>"+frontServers[i]+"</p></td>\n";
	string serverStatus = "<td>"+status1[i]+"</td>";
	string serverSelect = "<td><input type = \"submit\" value = \"start\">\n </td>\n</form></tr>\n" ;

	string total = formOpen + serverNum + serverName+serverStatus + serverSelect;
	firstTableBody = firstTableBody + total;

}
					string FirstTableEnd="</table></div>\n"
										"</div>\n";

				string secondTable="<div class=\"w3-row-padding w3-light-grey w3-padding-64 w3-container\">\n"
 " <div class=\"w3-content\">\n"
  "    <h1>BackEnd Servers</h1>\n";


				string secondTableStart = "</table>\n<h2>Back end server detials</h2>\n<table>\n<col width = \"150\"><col width = \"150\"><col width = \"150\"><tr>\n"
						"<th>server number</th>\n<th>server</th>\n<th>status</th>\n</tr>\n";
					string secondTableBody = "";
					for(int i = 0; i< status2.size(); i++)
					{
						string formOpen = "<tr>\n<form action =\"/startBackEndServer\" method = \"POST\"> ";
						string serverNum = "<td><input type=\"text\" name=\"serverSelected\" value ="+to_string(i)+" readonly></td>\n";
						string serverName = "<td><p>"+backServers[i]+"</p></td>\n";
						string serverStatus = "<td>"+status2[i]+"</td>";
						string serverSelect = "<td><input type = \"submit\" value = \"start\">\n </td>\n</form></tr>\n" ;

						string total = formOpen + serverNum + serverName+serverStatus + serverSelect;
						secondTableBody = secondTableBody + total;

					}
			string end=" </table></div>\n"
"</div>\n"
"</body>\n"
"</html>\n";

		return header+firstTableStart + firstTableBody+FirstTableEnd+secondTable+secondTableStart + secondTableBody+end;
}

// get server selected from postmessage 
int getNumber(string totalMesage)
{
		size_t user_pos = totalMesage.find("serverSelected=")+15;
		string server_num = totalMesage.substr(user_pos);
		//cout<<"server number "<<server_num<<endl;
		return stoi(server_num);
}
// get server selected from postmessage sent from server
int getServerSelected(string totalMesage)
{
		size_t user_pos = totalMesage.find("serverSelected=")+15;
		string server_num = totalMesage.substr(user_pos);
		//cout<<"server number "<<server_num<<endl;
		return stoi(server_num);
}
// get user selected from postmessage 
string getUserSelected(string totalMesage)
{
	size_t user_pos = totalMesage.find("nameSelected=")+13;
	size_t pos_end = totalMesage.find("&server");
	string userSelected = totalMesage.substr(user_pos, (pos_end-user_pos));
	return userSelected;
}
// get server selected from postmessage 
int getServerSelectedInFile(string totalMesage)
{
		size_t user_pos = totalMesage.find("serverSelected=")+15;
		size_t end_pos = totalMesage.find("&user");
		string server_num = totalMesage.substr(user_pos, (end_pos-user_pos));
		//cout<<"server number "<<server_num<<endl;
		return stoi(server_num);
}
// get user selected from postmessage 
string getUserSelectedInFile(string totalMesage)
{
	size_t user_pos = totalMesage.find("userName=")+9;
	//size_t pos_end = totalMesage.find("&server");
	string userSelected = totalMesage.substr(user_pos);
	return userSelected;
}
// get user selected from postmessage
string getUserSelectedInFolder(string totalMesage)
{
	size_t user_pos = totalMesage.find("userName=")+9;
	size_t pos_end = totalMesage.find("&folderName");
	string userSelected = totalMesage.substr(user_pos, (pos_end-user_pos));
	return userSelected;
}
// get folder name selected from postmessage
string getFolderName(string totalMesage)
{
	size_t user_pos = totalMesage.find("folderName=")+11;
	//size_t pos_end = totalMesage.find("&server");
	string folderSelected = totalMesage.substr(user_pos);
	return folderSelected;
}
// get file selected from postmessage
string getFileSelectedInFile(string totalMesage)
{
	size_t user_pos = totalMesage.find("nameSelected=")+13;
	size_t pos_end = totalMesage.find("&server");
	string userSelected = totalMesage.substr(user_pos, (pos_end-user_pos));
	return userSelected;
}
// clean up, text received from http
void cleanUpText(string& text){
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
	while((start_pos = text.find("%3B", start_pos)) != std::string::npos){
		text.replace(start_pos, 3, ";");
	}
}
// render html page to show all users on a server
string renderAllUsersPage(vector<string> users, int serverNumber, string address)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
								 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"

				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">Registered Users</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Back end server details of "+address+"</h1>\n";


	/*"</head>\n<body>\n<h1>Admin</h1>\n<h2>Back end server details of "+address+"</h2>\n";*/

	string body = "";
	for(int i = 0; i<users.size(); i++)
	{	string formOpen = "<form action =\"/getUserMails\" method = \"POST\"> ";
		string user = "<input type=\"text\" name=\"nameSelected\" value ="+users[i]+" readonly>\n";
		string hidden = "<input type = \"hidden\" name = \"serverSelected\" value =" + to_string(serverNumber) + ">\n";
		string selectedUser = "<input type = \"submit\" value = \"get user details\">\n </form>\n" ;
		string total = formOpen + user +hidden +selectedUser;
		body = body + total ;
	}
	string FirstTableEnd="</div>\n"
						"</div>\n";


		string end="</body>\n"
					"</html>\n";

		return header+body+FirstTableEnd+end;
}
// render html page to show all files of a user
string renderAllFiles(vector<string> files, int serverNumber, string userName, string address)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
				 				 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"

				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">User file Details</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Back end server details of "+address+"</h1>\n"

	"</head>\n<body>\n";

	string userSelected = "<h1>USER:  <input type=\"text\" name=\"userSelected\" value ="+userName+" readonly>\n</h1>\n";
		string body = "";
		for(int i = 0; i<files.size(); i++)
		{	string formOpen = "<form action =\"/getFileDetails\" method = \"POST\"> ";
			string fileName = "<input type=\"text\" name=\"nameSelected\" value ="+files[i]+" readonly>\n";
			string hiddenServerNumber = "<input type = \"hidden\" name = \"serverSelected\" value =" + to_string(serverNumber) + ">\n";
			string hiddenUserName = "<input type = \"hidden\" name = \"userName\" value =" + userName + ">\n";
			string selectedUser = "<input type = \"submit\" value = \"get file\">\n </form>\n" ;
			string total = formOpen + fileName + hiddenServerNumber + hiddenUserName + selectedUser;
			body = body + total ;
		}

	string FirstTableEnd="</div>\n"
						"</div>\n";


		string end="</body>\n"
					"</html>\n";

		return header+userSelected+body+FirstTableEnd+end;
}
// render html page for shwoing files in a folder selected
string renderFolderForFiles(vector<string> files, int serverNumber, string userName,string FileName, string address)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
								 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"

				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">Folder Details</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Back end server details of "+address+"</h1>\n"

	"</head>\n<body>\n";

	string userSelected = "<h1>USER:  <input type=\"text\" name=\"userSelected\" value ="+userName+" readonly>\n</h1>\n";
	string fileSelected = "<h4>FOLDER:  <input type=\"text\" name=\"FileSelected\" value ="+FileName+" readonly>\n</h4>\n";

		string body = "";
		for(int i = 0; i<files.size(); i++)
		{	string formOpen = "<form action =\"/getFolderFileDetails\" method = \"POST\"> ";
			string fileName = "<input type=\"text\" name=\"nameSelected\" value ="+files[i]+" readonly>\n";
			string hiddenServerNumber = "<input type = \"hidden\" name = \"serverSelected\" value =" + to_string(serverNumber) + ">\n";
			string hiddenUserName = "<input type = \"hidden\" name = \"userName\" value =" + userName + ">\n";
			string hiddenFolderName = "<input type = \"hidden\" name = \"folderName\" value =" + FileName + ">\n";
			string selectedUser = "<input type = \"submit\" value = \"get file\">\n </form>\n" ;
			string total = formOpen + fileName + hiddenServerNumber + hiddenUserName + hiddenFolderName+selectedUser;
			body = body + total ;
		}

	string FirstTableEnd="</div>\n"
						"</div>\n";


		string end="</body>\n"
					"</html>\n";

		return header+userSelected+fileSelected+body+FirstTableEnd+end;
}
// render html page of file selected on a server
string renderEachFile(vector<string> fileStructure, int serverNumber, string userName, string FileName, string address)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
								 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"

				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">Files Stored</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				               " <h1>Back end server details of "+address+"</h1>\n"

	"</head>\n<body>\n";

	string userSelected = "<h4>USER:  <input type=\"text\" name=\"userSelected\" value ="+userName+" readonly>\n</h4>\n";
		string fileSelected = "<h4>FILE:  <input type=\"text\" name=\"FileSelected\" value ="+FileName+" readonly>\n</h4>\n";
		string body = "";
		for(int i = 0; i<fileStructure.size(); i++)
		{
			string eachFile = "<p>" + fileStructure[i] + "</p>";
			body = body + eachFile;
		}

	string FirstTableEnd="</div>\n"
						"</div>\n";


		string end="</body>\n"
					"</html>\n";

		return header+userSelected+fileSelected+body+FirstTableEnd+end;
}
// render the load page of a selected front end server
string renderLoadPage(string load)
{
	string header = "<!DOCTYPE html>\n"
						"<html>\n"
                       "<title>Team T02 Admin</title>\n"
                     "<meta charset=\"UTF-8\">\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Lato\">\n"
					"<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Montserrat\">\n"
					"<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">\n"
					"<style>\n"
					"body,h1,h2,h3,h4,h5,h6 {font-family: \"Lato\", sans-serif}\n"
					".w3-bar,h1,button {font-family: \"Montserrat\", sans-serif}\n"
					".fa-anchor,.fa-coffee {font-size:200px}\n"
					"</style>\n"
					"<body>\n"
				"<div class=\"w3-top\">\n"
				 "  <div class=\"w3-bar w3-red w3-card-2 w3-left-align w3-large\">\n"
					"<a href=\"/\" class=\"w3-bar-item w3-button w3-padding-large w3-white\">Get Server Details</a>\n"
					"<a href=\"/quitServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Shut Down Server</a>\n"
				 	"<a href=\"/startServers\" class=\"w3-bar-item w3-button w3-hide-small w3-padding-large w3-hover-white\">Start Server</a>\n"

				 "</div>\n"
			   "</div>\n"
               "<header class=\"w3-container w3-red w3-center\" style=\"padding:128px 16px\">\n"
               "<h1 class=\"w3-margin w3-jumbo\">Load values</h1>\n"
               "</header>\n"
				"<div class=\"w3-row-padding w3-padding-64 w3-container\">\n"
				               " <div class=\"w3-content\">\n"
				              // " <h1>Back end server details of "+address+"</h1>\n"

	"</head>\n<body>\n";

	//string userSelected = "<h4>USER:  <input type=\"text\" name=\"userSelected\" value ="+userName+" readonly>\n</h4>\n";
	//	string fileSelected = "<h4>FILE:  <input type=\"text\" name=\"FileSelected\" value ="+FileName+" readonly>\n</h4>\n";
		string body = load;
	/*	for(int i = 0; i<fileStructure.size(); i++)
		{
			string eachFile = "<p>" + fileStructure[i] + "</p>";
			body = body + eachFile;
		}*/

	string FirstTableEnd="</div>\n"
						"</div>\n";


		string end="</body>\n"
					"</html>\n";

		return header+body+FirstTableEnd+end;
}
// forking backend server process from admin console
int startBackendServer(string address_file,string index)
{
	 int pid, status;
	   // first we fork the process
	   if (pid = fork()) {
	       // pid != 0: this is the parent process (i.e. our process)
	        // wait for the child to exit
	       //cout<<"in parent"<<endl;
	   } else {
	       /* pid == 0: this is the child process. now let's load the
	          "ls" program into this process and run it */

	       const char executable[] = "./server_bck";
	      // cout<<"in exec, running exec"<<endl;
	       // load it. there are more exec__ functions, try 'man 3 exec'
	       // execl takes the arguments as parameters. execv takes them as an array
	       // this is execl though, so:
	       //      exec         argv[0]  argv[1] end
	       execl(executable, executable,address_file.c_str(),index.c_str() ,NULL);

	       /* exec does not return unless the program couldn't be started.
	          when the child process stops, the waitpid() above will return.
	       */
	          //cout<<"exec returned"<<endl;

	   }
	   return status; // this is the parent process again.

}
// forking front end server from admin console
int startFrontendServer(string address_file,string index)
{
	 int pid, status;
	   // first we fork the process
	   if (pid = fork()) {
	       // pid != 0: this is the parent process (i.e. our process)
	        // wait for the child to exit
	      // cout<<"in parent"<<endl;
	   } else {
	       /* pid == 0: this is the child process. now let's load the
	          "ls" program into this process and run it */

	       const char executable[] = "./server_front";
	      // cout<<"in exec, running exec"<<endl;
	       // load it. there are more exec__ functions, try 'man 3 exec'
	       // execl takes the arguments as parameters. execv takes them as an array
	       // this is execl though, so:
	       //      exec         argv[0]  argv[1] end
	       execl(executable, executable,address_file.c_str(),index.c_str() ,NULL);

	       /* exec does not return unless the program couldn't be started.
	          when the child process stops, the waitpid() above will return.
	       */
	        //  cout<<"exec returned"<<endl;

	   }
	   return status; // this is the parent process again.

}
// thread for each request
void * worker(void *arg)
{

	// server details
	int comm_fd = *(int*)arg;
	char buf[1000];
	string strBuf = "";
	string tmp="";
	int ret,loc;
	string first_request = "";
	string postHeader = "";
	bool verify = 0;

	while(true)
	{
		ret = read(comm_fd, &buf, 1000);
		tmp = string(&buf[0],ret);
		strBuf.append(tmp);
		memset(buf,0,1000);

		while((loc = strBuf.find("\r\n\r\n"))!=string::npos)
		{
			// get first line from header
			int firstLineENd = strBuf.find("\r\n");
			string firstLine = strBuf.substr(0,firstLineENd);

			// get request type from first line
			int firstSpace = firstLine.find(" ");
			string requestType = firstLine.substr(0,firstSpace);	

			// find function name
			string rest_first_line = firstLine.substr(firstSpace+1);
			int space_index = rest_first_line.find(" ");
			string function_name = rest_first_line.substr(0, space_index);


			if(requestType == "GET")
			{
				//cout<<"get request"<<endl;
				//cout<<strBuf<<endl;
				// handle the first get request i.e., the home page of the admin-> shows all the servers and their statuses
				if(function_name == "/")
				{

					// send load requests and store their details
					// ping all front end servers 
					vector<string> frontendServerStatus;
					vector<string> frontendServerAddrs;
					//frontendServerStatus.push_back("none");
					for(int i = 0; i<frontServers.size(); i++)
					{
						// saving ip and port 
						string ipAndPort = get<0>(frontServers[i]);
						frontendServerAddrs.push_back(ipAndPort);
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);
						// setting up sockets
						int frontSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(frontSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in frontServaddr;
						bzero(&frontServaddr,sizeof(frontServaddr));
						frontServaddr.sin_family = AF_INET;
						frontServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(frontServaddr.sin_addr));

						connect(frontSockfd, (struct sockaddr*)&frontServaddr, sizeof(frontServaddr));

						// seding a LOAD request
						stringstream wsss;
						wsss << "LOAD\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"before sending data"<<endl;
    					sendDataNoReply(frontSockfd, (char*)headers.c_str(), headers.size());
    					//cout<<"after sending data"<<endl;
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";

    					int ret = read(frontSockfd, &temp, 1000);
    					//cout<<"after reading data"<<endl;
    					int load;
    					if(ret == -1)
    						frontendServerStatus.push_back("not-alive");
    					else
 							frontendServerStatus.push_back("alive");
					}
					// create a html page with status details
					vector<string> backendServerStatus;
					vector<string> backendServerAddrs;
					// ping all backend servers for status
					//backendServerStatus.push_back("none");
					for(int i =0; i<backServers.size(); i++)
					{
												// opening socket for the selected backend server
						string ipAndPort = get<0>(backServers[i]);
						backendServerAddrs.push_back(ipAndPort);
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /"<<endl;

						// set proto
						request req;

						req.set_request_type("GET");
						req.set_request_num(0);
						req.set_row("oshin");
						req.set_col("PASSWORD");
						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						//usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						if(out == -1)
							backendServerStatus.push_back("not-alive");
						else
							backendServerStatus.push_back("alive");
					}

					//cout<<adminTable<<endl;
					string adminTable = renderMainTable(frontendServerAddrs, frontendServerStatus, backendServerAddrs, backendServerStatus);
					const char* admin_chr = adminTable.c_str();
					int body_len = strlen(admin_chr);

					// setting headers
					stringstream wsss;
					wsss << "HTTP/1.1 200 OK\r\n" 
     				<< "Connection: keep-alive\r\n"
    				<< "Content-Type: text/html\r\n"
    	 			<< "Content-Length: " << body_len << "\r\n"
     	 			<< "\r\n";
     	 			string headers  = wsss.str();
    				sendData(comm_fd, (char*)headers.c_str(), headers.size());
    				sendData(comm_fd, (char*)adminTable.c_str(), adminTable.size());
					strBuf = strBuf.substr(loc+4);
				}
				else if(function_name == "/quitServers")
				{// handles the get request from admin to shutdown a server
					//cout<<"in quit servers"<<endl;
					vector<string> frontendServerStatus;
					vector<string> frontendServerAddrs;
					for(int i = 0; i<frontServers.size(); i++)
					{
						// saving ip and port 
						string ipAndPort = get<0>(frontServers[i]);
						frontendServerAddrs.push_back(ipAndPort);
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);
						// setting up sockets
						int frontSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(frontSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in frontServaddr;
						bzero(&frontServaddr,sizeof(frontServaddr));
						frontServaddr.sin_family = AF_INET;
						frontServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(frontServaddr.sin_addr));

						connect(frontSockfd, (struct sockaddr*)&frontServaddr, sizeof(frontServaddr));

						// seding a LOAD request
						stringstream wsss;
						wsss << "LOAD\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"before sending data"<<endl;
    					sendDataNoReply(frontSockfd, (char*)headers.c_str(), headers.size());
    					//cout<<"after sending data"<<endl;
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";

    					int ret = read(frontSockfd, &temp, 1000);
    					//cout<<"after reading data"<<endl;
    					int load;
    					if(ret == -1)
    						frontendServerStatus.push_back("not-alive");
    					else
 							frontendServerStatus.push_back("alive");
					}
					// create a html page with status details
					vector<string> backendServerStatus;
					vector<string> backendServerAddrs;
					// ping all backend servers for status
					for(int i =0; i<backServers.size(); i++)
					{
												// opening socket for the selected backend server
						string ipAndPort = get<0>(backServers[i]);
						backendServerAddrs.push_back(ipAndPort);
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /quitservers"<<endl;


						// set proto
						request req;

						req.set_request_type("GET");
						req.set_request_num(0);
						req.set_row("oshin");
						req.set_col("PASSWORD");
						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						//usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						if(out == -1)
							backendServerStatus.push_back("not-alive");
						else
							backendServerStatus.push_back("alive");
					}

					//cout<<adminTable<<endl;
					string adminTable = renderQuitPage(frontendServerAddrs,frontendServerStatus,backendServerAddrs,backendServerStatus);
					const char* admin_chr = adminTable.c_str();
					int body_len = strlen(admin_chr);

					// setting headers
					stringstream wsss;
					wsss << "HTTP/1.1 200 OK\r\n" 
     				<< "Connection: keep-alive\r\n"
    				<< "Content-Type: text/html\r\n"
    	 			<< "Content-Length: " << body_len << "\r\n"
     	 			<< "\r\n";
     	 			string headers  = wsss.str();
    				sendData(comm_fd, (char*)headers.c_str(), headers.size());
    				sendData(comm_fd, (char*)adminTable.c_str(), adminTable.size());
					strBuf = strBuf.substr(loc+4);
				}
				else if(function_name == "/startServers")
				{// handles a get request to show a pge to start servers
					//cout<<"in start servers"<<endl;
					vector<string> frontendServerStatus;
					vector<string> frontendServerAddrs;
					for(int i = 0; i<frontServers.size(); i++)
					{
						// saving ip and port 
						string ipAndPort = get<0>(frontServers[i]);
						frontendServerAddrs.push_back(ipAndPort);
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);
						// setting up sockets
						int frontSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(frontSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in frontServaddr;
						bzero(&frontServaddr,sizeof(frontServaddr));
						frontServaddr.sin_family = AF_INET;
						frontServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(frontServaddr.sin_addr));

						connect(frontSockfd, (struct sockaddr*)&frontServaddr, sizeof(frontServaddr));

						// seding a LOAD request
						stringstream wsss;
						wsss << "LOAD\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"before sending data"<<endl;
    					sendDataNoReply(frontSockfd, (char*)headers.c_str(), headers.size());
    					//cout<<"after sending data"<<endl;
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";

    					int ret = read(frontSockfd, &temp, 1000);
    					//cout<<"after reading data"<<endl;
    					int load;
    					if(ret == -1)
    						frontendServerStatus.push_back("not-alive");
    					else
 							frontendServerStatus.push_back("alive");
					}
					// create a html page with status details
					vector<string> backendServerStatus;
					vector<string> backendServerAddrs;
					// ping all backend servers for status
					for(int i =0; i<backServers.size(); i++)
					{
												// opening socket for the selected backend server
						string ipAndPort = get<0>(backServers[i]);
						backendServerAddrs.push_back(ipAndPort);
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /startservers"<<endl;

						// set proto
						request req;

						req.set_request_type("GET");
						req.set_request_num(0);
						req.set_row("oshin");
						req.set_col("PASSWORD");
						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						//usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
					//	cout<<out<<"   bytes from proto "<<endl;
						if(out == -1)
							backendServerStatus.push_back("not-alive");
						else
							backendServerStatus.push_back("alive");
					}

					//cout<<adminTable<<endl;
					string adminTable = renderStartPage(frontendServerAddrs,frontendServerStatus,backendServerAddrs,backendServerStatus);
					const char* admin_chr = adminTable.c_str();
					int body_len = strlen(admin_chr);

					// setting headers
					stringstream wsss;
					wsss << "HTTP/1.1 200 OK\r\n" 
     				<< "Connection: keep-alive\r\n"
    				<< "Content-Type: text/html\r\n"
    	 			<< "Content-Length: " << body_len << "\r\n"
     	 			<< "\r\n";
     	 			string headers  = wsss.str();
    				sendData(comm_fd, (char*)headers.c_str(), headers.size());
    				sendData(comm_fd, (char*)adminTable.c_str(), adminTable.size());
					strBuf = strBuf.substr(loc+4);
				}

				else
				{
					stringstream wsss;
					wsss << "HTTP/1.1 204\r\n" 
					<<"Content-Length: 0\r\n"
     	 			<< "\r\n";

     	 			string headers  = wsss.str();
    				sendData(comm_fd, (char*)headers.c_str(), headers.size());

					strBuf = strBuf.substr(loc+4);
				}
				
			}
			// not handling any POST request for now
			else if(requestType == "POST")	//
			{
				//cout<<"post request"<<endl;
				if(function_name == "/getLoad")
				{// get load to determine status from front end server
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						string totalMesage = postHeader + postMessage;
						int serverNumber = getNumber(totalMesage);
						//cout<<serverNumber<<" dnej "<<endl;

						// opening socket for the selected frontend server
						string ipAndPort = get<0>(frontServers[serverNumber]);
						string thisAddress = ipAndPort;
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to frontend"<<endl;

						// send request
						// sending a LOAD request
						stringstream wsss;
						wsss << "LOAD\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"count in get "<<count<<endl;
    					sendDataNoReply(backSockfd, (char*)headers.c_str(), headers.size());
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";
    					int ret = read(backSockfd, &temp, 1000);
    					string loadDetected(temp);
    					bool load = 0;
    					if(ret == -1)
    						loadDetected = "-1";
    					else
    					{
    						// read load details
							loadDetected = loadDetected;

    					}
    					//cout<<"load is "<<load<<endl;

						close(backSockfd);

						 //= "Server "+ to_string(serverNumber) + " has " + to_string(load) + " connections running";
						string serverLoad = "The number of connections open on server number "+
						 thisAddress + " is " + loadDetected;
						string serverDetails = renderLoadPage(serverLoad);

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);

						// setting headers
						stringstream head;
						head << "HTTP/1.1 200 OK\r\n" 
     					<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    	 				<< "Content-Length: " << body_len << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################

						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverNumber = -1;
					}
					else
					{
						postMessage = "";

					}
				}

				else if(function_name == "/getLoadBack")
				{// get load from backend server to determine server status
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						//cout<<strBuf<<endl;
						string totalMesage = postHeader + postMessage;
						int serverNumber = getNumber(totalMesage);
						//cout<<serverNumber<<" dnej "<<endl;

						string ipAndPort = get<0>(backServers[serverNumber]);
						string thisAddress = ipAndPort;
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /getLoadBack"<<endl;

						// set proto
						request req;

						req.set_request_type("GET_ROWS");
						req.set_request_num(0);
						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						usleep(500);
						// include receive message
						//cout<<"before receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						vector<string> allUsers;
						//cout<<receivedReq.val1_size()<<"  val1 size"<<endl;
						for(int i = 0; i<receivedReq.val1_size(); i++)
						{
							string user = receivedReq.val1(i);
							allUsers.push_back(user);
						}

						close(backSockfd);
						string serverDetails = renderAllUsersPage(allUsers, serverNumber, thisAddress);

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);

						// setting headers
						stringstream head;
						head << "HTTP/1.1 200 OK\r\n" 
     					<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    	 				<< "Content-Length: " << body_len << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################

						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverNumber = -1;
					}
					else
					{
						postMessage = "";

					}
				}
			
				else if(function_name == "/getUserMails")//
				{// gets all the mails beloning to a particualr user from a backend server
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					//cout<<"before conlen"<<endl;
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						string totalMesage = postHeader + postMessage;
						//cout<<totalMesage<<endl;
						//cout<<"before getting server selected"<<endl;
						int serverSelected = getServerSelected(totalMesage);
						//cout<<"after server selected"<<endl;
						string userSelected = getUserSelected(totalMesage);
						//cout<<serverNumber<<" dnej "<<endl;

						string ipAndPort = get<0>(backServers[serverSelected]);
						string thisAddress = ipAndPort;
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /getusermails"<<endl;

						// set proto
						request req;
						//cout<<userSelected<<endl;

						req.set_request_type("GET_COLS");
						req.set_request_num(0);
						req.set_row(userSelected);

						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						vector<string> userFiles;
						for(int i = 0; i<receivedReq.val1_size(); i++)
						{
							string file = receivedReq.val1(i);
							userFiles.push_back(file);
						}

						close(backSockfd);
						string serverDetails = renderAllFiles(userFiles, serverSelected,userSelected, thisAddress);

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);

						// setting headers
						stringstream head;
						head << "HTTP/1.1 200 OK\r\n" 
     					<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    	 				<< "Content-Length: " << body_len << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################

						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverSelected = -1;
					}
					else
					{
						postMessage = "";

					}
				}

				else if(function_name == "/getFolderFileDetails")//
				{// gets all the files present in a particular folder from a backend server
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					//cout<<"before conlen"<<endl;
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						string totalMesage = postHeader + postMessage;
						//cout<<totalMesage<<endl;
						string fileSelected = getFileSelectedInFile(totalMesage);
						//cout<<"before getting server selected"<<endl;
						int serverSelected = getServerSelectedInFile(totalMesage);
						//cout<<"after server selected"<<endl;
						string userSelected = getUserSelectedInFolder(totalMesage);
						//cout<<"after folder name"<<endl;
						string folderName = getFolderName(totalMesage);
						//cout<<fileSelected<<"  "<<serverSelected<<"  "<<userSelected<<"  " <<folderName<<endl;
						//cout<<serverNumber<<" dnej "<<endl;
						cleanUpText(fileSelected);
						cleanUpText(folderName);
						//cout<<"file selected after clean up"<<fileSelected<<""  <<folderName<<endl;
						string ipAndPort = get<0>(backServers[serverSelected]);
						string thisAddress = ipAndPort;
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /in getfilefolderdetails"<<endl;
						string actualFIle = folderName + fileSelected;
						// set proto
						request req;

						req.set_request_type("GET");
						req.set_request_num(0);
						req.set_row(userSelected);
						req.set_col(actualFIle);

						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						vector<string> allFiles;
						for(int i = 0; i<receivedReq.val1_size(); i++)
						{
							//cout<<"entered"<<endl;
							string file = receivedReq.val1(i);
							allFiles.push_back(file);
						}

						close(backSockfd);

						if(true)//allFiles.size() == 1)
						{
							string serverDetails = renderEachFile(allFiles, serverSelected,userSelected, actualFIle, thisAddress );
							const char* server_chr = serverDetails.c_str();
							int body_len = strlen(server_chr);

							// setting headers
							stringstream head;
							head << "HTTP/1.1 200 OK\r\n" 
     						<< "Connection: keep-alive\r\n"
    						<< "Content-Type: text/html\r\n"
    	 					<< "Content-Length: " << body_len << "\r\n"
     	 					<< "\r\n";
     	 					string postheaders  = head.str();
    						sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    						sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################
    					}
    					else if(false)//allFiles.size() > 1)
    					{
    						string serverDetails = renderFolderForFiles(allFiles, serverSelected,userSelected, fileSelected, thisAddress );
							const char* server_chr = serverDetails.c_str();
							int body_len = strlen(server_chr);

							// setting headers
							stringstream head;
							head << "HTTP/1.1 200 OK\r\n" 
     						<< "Connection: keep-alive\r\n"
    						<< "Content-Type: text/html\r\n"
    	 					<< "Content-Length: " << body_len << "\r\n"
     	 					<< "\r\n";
     	 					string postheaders  = head.str();
    						sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    						sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
	
    					}
						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverSelected = -1;
					}
					else
					{
						postMessage = "";

					}
				}


				else if(function_name == "/getFileDetails")//
				{// gets all the detials of a particular file selected from a backend server
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					//cout<<"before conlen"<<endl;
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						string totalMesage = postHeader + postMessage;
						//cout<<totalMesage<<endl;
						string fileSelected = getFileSelectedInFile(totalMesage);
						//cout<<"before getting server selected"<<endl;
						int serverSelected = getServerSelectedInFile(totalMesage);
						//cout<<"after server selected"<<endl;
						string userSelected = getUserSelectedInFile(totalMesage);
					//	cout<<fileSelected<<"  "<<serverSelected<<"  "<<userSelected<<endl;
						//cout<<serverNumber<<" dnej "<<endl;
						cleanUpText(fileSelected);
						//cout<<"file selected after clean up"<<fileSelected<<endl;
						string ipAndPort = get<0>(backServers[serverSelected]);
						string thisAddress = ipAndPort;
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /getfiledetails"<<endl;

						// set proto
						request req;

						req.set_request_type("GET");
						req.set_request_num(0);
						req.set_row(userSelected);
						req.set_col(fileSelected);

						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						vector<string> allFiles;
						for(int i = 0; i<receivedReq.val1_size(); i++)
						{
							//cout<<"entered"<<endl;
							string file = receivedReq.val1(i);
							allFiles.push_back(file);
						}

						close(backSockfd);


    					if(allFiles.size() > 1)
    					{
    						string serverDetails = renderFolderForFiles(allFiles, serverSelected,userSelected, fileSelected, thisAddress );
							const char* server_chr = serverDetails.c_str();
							int body_len = strlen(server_chr);

							// setting headers
							stringstream head;
							head << "HTTP/1.1 200 OK\r\n" 
     						<< "Connection: keep-alive\r\n"
    						<< "Content-Type: text/html\r\n"
    	 					<< "Content-Length: " << body_len << "\r\n"
     	 					<< "\r\n";
     	 					string postheaders  = head.str();
    						sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    						sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
	
    					}
    					else
						{
							string serverDetails = renderEachFile(allFiles, serverSelected,userSelected, fileSelected, thisAddress );
							const char* server_chr = serverDetails.c_str();
							int body_len = strlen(server_chr);

							// setting headers
							stringstream head;
							head << "HTTP/1.1 200 OK\r\n" 
     						<< "Connection: keep-alive\r\n"
    						<< "Content-Type: text/html\r\n"
    	 					<< "Content-Length: " << body_len << "\r\n"
     	 					<< "\r\n";
     	 					string postheaders  = head.str();
    						sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    						sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################
    					}
						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverSelected = -1;
					}
					else
					{
						postMessage = "";

					}
				}


				else if(function_name == "/startBackEndServer")
				{// sends request to backend servr to start it
					//cout<<"in back server start"<<endl;

					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						//cout<<strBuf<<endl;
						string totalMesage = postHeader + postMessage;
						int serverNumber = getNumber(totalMesage)+1;
						//cout<<serverNumber<<endl;
						//cout<<serverNumber<<" dnej "<<endl;
						int status=-2;
						//cout<<"before staring exec"<<endl;
						status=startBackendServer("back_addr.txt",to_string(serverNumber));
						//cout<<"after exec"<<endl;
						if(status== -1)
						{
							//cout<<"Error Starting the server"<<endl;
						}

						string serverDetails = "";

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);
						string quitAddress = adminDefaultIp + to_string(adminPort) + "/startServers";
						// setting headers
						stringstream head;
						head << "HTTP/1.1 302 Found\r\n" 
     					//<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    					<< "Location: http://"<<quitAddress<<"\r\n"
    	 				<< "Content-Length: " << 0 << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());

					
						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverNumber = -1;
					}
					else
					{
						postMessage = "";

					}
				}

				else if(function_name == "/startFrontEndServer")
				{// sends requst to front end server to start it
					//cout<<"in front server start"<<endl;
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						//cout<<strBuf<<endl;
						string totalMesage = postHeader + postMessage;
						int serverNumber = getNumber(totalMesage)+1;
						//cout<<serverNumber<<endl;
						//cout<<serverNumber<<" dnej "<<endl;
						int status=-2;
						//cout<<"before staring exec"<<endl;
						status=startFrontendServer("front_addr.txt",to_string(serverNumber));
						//cout<<"after exec"<<endl;
						if(status== -1)
						{
							//cout<<"Error Starting the server"<<endl;
						}

						string serverDetails = "";

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);
						string quitAddress = adminDefaultIp + to_string(adminPort) + "/startServers";
						// setting headers
						stringstream head;
						head << "HTTP/1.1 302 Found\r\n" 
     					//<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    					<< "Location: http://"<<quitAddress<<"\r\n"
    	 				<< "Content-Length: " << 0 << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());

					
						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverNumber = -1;
					}
					else
					{
						postMessage = "";

					}
				}	
				else if(function_name == "/quitFrontEndServer")
				{// sends request to front end server to stop it 
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
					//	cout<<strBuf<<endl;
						string totalMesage = postHeader + postMessage;
						int serverNumber = getNumber(totalMesage);
						//cout<<serverNumber<<" dnej "<<endl;

						string ipAndPort = get<0>(frontServers[serverNumber]);
					//	cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to frontend"<<endl;

						stringstream wsss;
						wsss << "QUIT\r\n" 
     	 				<< "\r\n";
     	 				string headers  = wsss.str();
	   	 				//cout<<"before sending data"<<endl;
    					sendDataNoReply(backSockfd, (char*)headers.c_str(), headers.size());
    					//cout<<"after sending data"<<endl;
    					// wait
    					usleep(500);
    					// receive response
    					char temp[1000];
    					string tmp = "";

    					int ret = read(backSockfd, &temp, 1000);
    					//cout<<"after reading data"<<endl;


						close(backSockfd);
						string serverDetails = "";

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);
						string quitAddress = adminDefaultIp + to_string(adminPort) + "/startServers";
						// setting headers
						stringstream head;
						head << "HTTP/1.1 302 Found\r\n" 
     					//<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    					<< "Location: http://"<<quitAddress<<"\r\n"
    	 				<< "Content-Length: " << 0 << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################

						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverNumber = -1;
					}
					else
					{
						postMessage = "";

					}
				}		
				else if("/quitBackEndServer")
				{// sends request to back end server to stop it
					int clpos=strBuf.find("Content-Length: ");
					string temp = strBuf.substr(clpos+16);
					int crlf = temp.find("\r\n");
					int con_len = stoi(temp.substr(0,crlf));
					//cout<<con_len<<endl;
					//string messageBody = strBuf.substr()
					//////////////////////////////////////////////
					postHeader = strBuf.substr(0,loc+4);
					string postMessage = strBuf.substr(loc+4);
					if(con_len == postMessage.size())
					{
						// ########################################################################
						// frontend server getDetails
						// getting server number from post 
						//cout<<strBuf<<endl;
						string totalMesage = postHeader + postMessage;
						int serverNumber = getNumber(totalMesage);
						//cout<<serverNumber<<" dnej "<<endl;

						string ipAndPort = get<0>(backServers[serverNumber]);
						//cout<<ipAndPort<<endl;
						char *ipAndPortChar = &ipAndPort[0u];
						char * ip = strtok(ipAndPortChar, ":");
						char * frontstringPort = strtok(NULL, ":");
						int frontPort = stoi(frontstringPort, NULL, 10);

						// setting up sockets
						int backSockfd = socket(PF_INET, SOCK_STREAM, 0);
						if(backSockfd < 0)
						{
							fprintf(stderr, "cannot open socket\n");
						}
						struct sockaddr_in backServaddr;
						bzero(&backServaddr,sizeof(backServaddr));
						backServaddr.sin_family = AF_INET;
						backServaddr.sin_port = htons(frontPort);
						inet_pton(AF_INET, ip, &(backServaddr.sin_addr));

						connect(backSockfd, (struct sockaddr*)&backServaddr, sizeof(backServaddr));
						//cout<<"connected to backedn"<<endl;
						//cout<<"in the /quitbakcendservr"<<endl;

						// set proto
						request req;

						req.set_request_type("QUIT");
						req.set_request_num(0);
						// 
						//cout<<"sending proto message"<<endl;
						// include send function
						sendmsgProto(backSockfd,req);
						// sleep for .5 sec
						usleep(500);
						// include receive message
						//cout<<"receiving proto message"<<endl;
						request receivedReq;
						int out = readmsg(backSockfd, receivedReq);
						// check if output is present
						//cout<<out<<"   bytes from proto "<<endl;
						//cout<<"just quit this server"<<endl;
						close(backSockfd);
						string serverDetails = "";

						const char* server_chr = serverDetails.c_str();
						int body_len = strlen(server_chr);
						string quitAddress = adminDefaultIp + to_string(adminPort) + "/quitServers";
						// setting headers
						stringstream head;
						head << "HTTP/1.1 302 Found\r\n" 
     					//<< "Connection: keep-alive\r\n"
    					<< "Content-Type: text/html\r\n"
    					<< "Location: http://"<<quitAddress<<"\r\n"
    	 				<< "Content-Length: " << 0 << "\r\n"
     	 				<< "\r\n";
     	 				string postheaders  = head.str();
    					sendData(comm_fd, (char*)postheaders.c_str(), postheaders.size());
    					sendData(comm_fd, (char*)serverDetails.c_str(), serverDetails.size());
    					// #############################################################################

						strBuf = "";
    					postMessage = "";
    					postHeader = "";
    					totalMesage = "";
    					serverNumber = -1;
					}
					else
					{
						postMessage = "";

					}
				}

				

				
			}	

			else
			{
				//cout<<"other request"<<endl;
			}
		}
	}
}


int main(int argc, char *argv[])
{
	int port = 0;

	if(argc !=4)
	{
		fprintf(stderr,"provide backend_addressfile, frontend_addressfile, portnumber");
		exit(1);
	}
	string BackaddrFile = argv[1];
	string FrontaddrFile = argv[2];
	port = stoi(argv[3]);


	adminPort = port;

		// open and store front end server detials 
	ifstream frontfile(FrontaddrFile);
	if(!frontfile.is_open())
	{
		fprintf(stderr, "unable to open file\n");
		exit(1);
	}
	string frontline;
	// storing frontend server details in a vector
	bool skipFront = 1;
	while(getline(frontfile, frontline))
	{
		if(skipFront == 0)
			frontServers.push_back(make_tuple(frontline, 0));
		skipFront = 0;
	}

	// open and store backend server details
	
	ifstream backfile(BackaddrFile);
	if(!backfile.is_open())
	{
		fprintf(stderr, "unable to open file\n");
		exit(1);
	}
	string backline;
	int count = 0;
	// storing backend server details in a vector
	bool skipBack = 1;
	while(getline(backfile, backline))
	{

		if(skipBack == 0)
			backServers.push_back(make_tuple(backline, 0));
		skipBack = 0;
	}

	//cout<<backServers.size()<<" back server size"<<endl;

	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	//printf("port number is %d\n",port);
	servaddr.sin_port=htons(port);
	bind(listen_fd,(struct sockaddr*)&servaddr, sizeof(servaddr));

	char *adminIp = inet_ntoa(servaddr.sin_addr);
	//cout<<adminIp<<endl;

	listen(listen_fd, 100);
	while(true){
		
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int *fd = (int*)malloc(sizeof(int));
		*fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
		//all_threads[t_num] = *fd;//change to vectorfs
		//t_num++;

		if(v==true)
		{	
			fprintf(stderr, "New connection\n");	
			//cout<<"new connection after counting : "<<count<<endl;
		}
		//cout<<fd<<"  file desriptor value"<<endl;
		pthread_t thread;
		//pthread_create(&thread, NULL, worker, fd);
		pthread_create(&thread, NULL, worker, fd);
	}
	return 0;
}
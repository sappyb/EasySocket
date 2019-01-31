/************************************HEADER FILES*************************************************/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <list>
#include <iterator>
#include <arpa/inet.h>
#include <algorithm>
#include <map>
/**************************************MACROS*****************************************************/
#define BUFFER 4096
/*************************************************************************************************/
using namespace std;
/********************************GLOBAL DATA STRUCTURES*******************************************/
//map<string,User> allUsers;
//map<string,User> onlineUsers;

/*********************************FUNTION PROTOTYPES***********************************************/
vector<string> parse_String(string,char);
vector<string> read_File(fstream&);
static void signalHandler(int);
bool parseClientMessage(int sockfd,char *buf);
bool userLogin(string messageBody);
string createLoginAndRegResponseString(string opType,string statusCode);
void logoutUser(string messageBody);
string getPeerAddressFromSock(int sockfd);
/**************************************************************************************************/

class User
{

public:
    string username;
    string password;
    int sockfd;
    bool online;
    int listeningPort;
    string address;
    vector<string> friendList;
    vector<string> onlineFriendList;
    vector<string> inviteList;
    	
    User(string userConfigLine)
    {
        string friends,s;
        stringstream f(userConfigLine);

        getline(f, username, '|');
        getline(f, password, '|');
        getline(f, friends, '|');

        stringstream friendStream(friends);
        while (getline(friendStream, s, ';'))
        {
            friendList.push_back(s);
	    inviteList.push_back(s);

        }	
	online = false;
	sockfd=-1;
	listeningPort=-1;
	address="";
	onlineFriendList.clear();
	inviteList.clear();
    }
    User(string uname, string pass)
    {
        username=uname;
        password=pass;
        friendList.clear();
	onlineFriendList.clear();
	sockfd=-1;
	listeningPort=-1;
	address="";
	online=false;
	inviteList.clear();
    }

    User()
    {
        username="";
        password="";
        friendList.clear();
	onlineFriendList.clear();
	sockfd=-1;
	listeningPort=-1;
	address="";
	online=false;
	inviteList.clear();
    }

    void setOnline(bool status){
    	online = status;
    }
    bool getOnline(){	
    	return online;
    }
    void setPassword(string mypassword){
    	password=mypassword;
    }
    string getPassword(){
    	return password;
    }
    void setUsername(string myusername){
    	username=myusername;
    }
    string getUsername(){
   	return username;
    }
    void setAddress(string myaddress){
    	address=myaddress;
    }
    string getAddress(){
    	return address;
    }
    void setSockfd(int mysockfd){
    	sockfd=mysockfd;
    }
    int getSockfd(){
   	return sockfd;
    }
    void setListeningPort(int mylisteningport){
    	listeningPort=mylisteningport;
    }
    int getListeningPort(){
	return listeningPort;
    }
    vector<string> getFriendlist(){
    	return friendList;
    }
    vector<string> getOnlineFriendlist(){
    	return onlineFriendList;
    }
    
    void addFriendlist(string myfriend){
	int flag = 0;
    	auto it = friendList.begin();
	while(it != friendList.end()){
		if(*it == myfriend){
			flag=1;
			break;
		}
		it++;
	}
	if(flag == 0)
		friendList.push_back(myfriend);
    }
    void addOnlineFriendlist(string myfriend){
	int flag = 0;
    	auto it = onlineFriendList.begin();
	while(it != onlineFriendList.end()){
		if(*it == myfriend){
			flag=1;
			break;
		}
		it++;
	}
	if(flag == 0)
		onlineFriendList.push_back(myfriend);
    }
    void addInvitelist(string myfriend){
	int flag = 0;
    	auto it = inviteList.begin();
	while(it != inviteList.end()){
		if(*it == myfriend){
			flag=1;
			break;
		}
		it++;
	}
	if(flag == 0)
		inviteList.push_back(myfriend);
    }
    void printClassVariables(){
	cout<<"Username :"<<username<<endl;
	cout<<"Password :"<<password<<endl;
	cout<<"Socket Descriptor :"<<sockfd<<endl;
	cout<<"Address :"<<address<<endl;
	cout<<"Listening Port :"<<listeningPort<<endl;
	cout<<"Online Status :"<<online;
	cout<<"The friendlist is :"<<endl;
	for(auto it = friendList.begin();it != friendList.end(); it++){
		cout<<*it<<"||";
	}
	cout<<"The online friendlist is :"<<endl;
	for(auto it = onlineFriendList.begin();it != onlineFriendList.end(); it++){
		cout<<*it<<"||";
	}
	for(auto it = inviteList.begin();it != inviteList.end(); it++){
		cout<<*it<<"||";
	}
    }
    void userLogout(){
    	onlineFriendList.clear();
	address.clear();
	sockfd=-1;
	listeningPort=-1;
	online=false;
    }

};
fd_set allset, rset;
int sockfd, rec_sock;
map<string,User> allUsers;
map<string,User> onlineUsers;

int main(int argc, char ** argv){
	struct sigaction interruptHandler;
	typedef struct hostent hostent;
	interruptHandler.sa_handler = signalHandler;
        sigemptyset(&interruptHandler.sa_mask);
        interruptHandler.sa_flags = 0;
        if((sigaction(SIGINT, &interruptHandler, NULL)) == -1){
		perror("Error: Error while catching the signal :");
		exit(EXIT_FAILURE);
	}
	if(argc < 2){//Checking for the number of Arguments.
		cout<<"Wrong Arguments Entered"<<endl;
		cout<<"The correct form of entering the arguments are"<<endl;
		cout<<"./messenger_server user_info server_config";
		exit(EXIT_FAILURE);
	}
	vector<string> userInfoFileLines, serverConfigFileLines;
	fstream userFileFd, serverFileFd;
	userFileFd.open(argv[1],ios::in);
	if(!userFileFd){//Checking if the file containing the User Information is opened properly.
		cout<<"Error: Error in openinf the file";
	}else{
		userInfoFileLines = read_File(userFileFd);
	}
	userFileFd.close();
	auto userfileit = userInfoFileLines.begin();
	while(userfileit!=userInfoFileLines.end()){//add the user in the allUser vector
		User newuser(*userfileit);
		allUsers.insert({newuser.username,newuser});
		userfileit++;
	}
	serverFileFd.open(argv[2],ios::in);
	if(!serverFileFd){//Checking if the file containing the Server Configuration is opened properly.
		cout<<"Error: Error in openinf the file";
	}else{
		serverConfigFileLines = read_File(serverFileFd);
	}
	serverFileFd.close();
	struct sockaddr_in addr, recaddr;
    	char buf[BUFFER];
    //	fd_set allset, rset;
    	int maxfd;
	socklen_t len;
	vector<string> serverConfigurationFetch;
	int port;
	//int sockfd, rec_sock;
	serverConfigurationFetch = parse_String(serverConfigFileLines.at(0),':');
	if(((serverConfigurationFetch.at(0)).compare("port") == 0) || ((serverConfigurationFetch.at(0)).compare("Port") == 0) || ((serverConfigurationFetch.at(0)).compare("PORT") == 0)){
		port = stoi(serverConfigurationFetch.at(1));
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	{
        	perror("Error: Unable to find the socket : ");
        	exit(EXIT_FAILURE);
    	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
    	addr.sin_addr.s_addr = INADDR_ANY;
    	addr.sin_family = AF_INET;
    	addr.sin_port = htons((short)port);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    	{
        	perror("Error: Error in binding the socket: ");
        	exit(EXIT_FAILURE);
    	}

    	char hostname[BUFFER];
	hostent *host;
    	if(gethostname(hostname, BUFFER) == -1){
        	perror("Error: Unable to get hostname: ");
        	exit(EXIT_FAILURE);
    	}
    	if((host = gethostbyname(hostname)) == NULL){
		perror("Error: Error occured while retriving the host information :");
		exit(EXIT_FAILURE);
	}
    	cout<< "Server is starting"<< flush<<endl;
    	cout<< "Server:"<< host->h_name<< flush<< endl;

	len = sizeof(addr);
	if (getsockname(sockfd, (struct sockaddr *)&addr, (socklen_t *)&len) < 0){
		perror("Error: can't get name");
		exit(EXIT_FAILURE);
	}
	printf("\nPort:%d\n", htons(addr.sin_port));

    	if (listen(sockfd, 5) < 0)
    	{
        	perror("Error: Bind");
        	exit(EXIT_FAILURE);
    	}
	FD_ZERO(&rset);
    	FD_ZERO(&allset);
    	FD_SET(sockfd, &allset);
    	maxfd = sockfd;

   	vector<int> clientSockets;
	clientSockets.clear();

	while (1)
    	{
        	rset = allset;
        	select(maxfd+1, &rset, NULL, NULL, NULL);
        	if (FD_ISSET(sockfd, &rset))
        	{
            		len = sizeof(recaddr);
            		if ((rec_sock = accept(sockfd, (struct sockaddr *)(&recaddr), &len)) < 0)
            		{
                		if (errno == EINTR)
                    			continue;
                		else{
                    			perror("Error: Connection accept error");
                    			exit(EXIT_FAILURE);
                		}
            		}
            		if (rec_sock < 0)
            		{
                		perror("Error: Accept");
                		exit(EXIT_FAILURE);
            		}
          	 	printf("Connected Remote Machine = %s | Port = %d.\n",inet_ntoa(recaddr.sin_addr), ntohs(recaddr.sin_port));
			clientSockets.push_back(rec_sock);
            		FD_SET(rec_sock, &allset);
            		if (rec_sock > maxfd){
				maxfd = rec_sock;
			}
        	}
		vector<int>::iterator it;
		it = clientSockets.begin();
        	while (it != clientSockets.end())
        	{
            		int num, fd;
            		fd = *it;
            		if (FD_ISSET(fd, &rset))
            		{
                		num = read(fd, buf, BUFFER);
                		if (num == 0)
                		{
                    			close(fd);
                    			FD_CLR(fd, &allset);
                    			it = clientSockets.erase(it);
                    			User user;
    					auto itra=onlineUsers.begin();
    					while(itra!=onlineUsers.end())
    					{
        					user=itra->second;
        					if(user.sockfd==fd)
        					{
							user.setOnline(false);
            						itra=onlineUsers.erase(itra);
            						cout<<"Current Online Users:"<<onlineUsers.size()<<endl;
            						break;
        					}
        					++itra;
    					}
                    			continue;
                		}
                		else
                		{	
                    			bool islogout=parseClientMessage(fd,buf);
                    			if(islogout)
                    			{
        //                			continue;
                    			}
                		}
            		}
            		++it;
        	}	
        	maxfd = sockfd;
        	if (!clientSockets.empty())
        	{
            		maxfd = max(maxfd, *max_element(clientSockets.begin(), clientSockets.end()));
        	}
    	}

}


vector<string> read_File(fstream& fileHandler){
	vector<string> fileLines;
	string line;
	while(getline(fileHandler,line)){
		fileLines.push_back(line);
	}
	return fileLines;
}
vector<string> parse_String(string line,char delimiter){
        vector<string> wordVector;
        stringstream stringStream(line);
	string token;
	while(getline(stringStream, token, delimiter)){
		wordVector.push_back(token);
	}
	 return wordVector;
}

bool parseClientMessage(int sockfd,char *buf)
{
    string sbuf(buf);
    stringstream message(sbuf);
    string messageBody;
    getline(message,messageBody,'~');
    if(messageBody.compare("login")==0)
    {
	int flag = 0;
	string mystring,mystringuser,mystringpass,username,password,response,bodyMsg; 
        getline(message,bodyMsg,'~');
	stringstream mymessageBody(bodyMsg);
	getline(mymessageBody,mystringuser,'|');
	stringstream valueUser(mystringuser);
	getline(mymessageBody,mystringpass,'|');
	stringstream valuepass(mystringpass);
	getline(valueUser,mystring,':'); 
        if(mystring.compare("username")==0)
        {
            getline(valueUser,username,':');
        }
	getline(valuepass,mystring,':');
	
        if(mystring.compare("password")==0)
        {
            getline(valuepass,password,':');
        }
        mymessageBody.clear();
	valuepass.clear();
	valueUser.clear();
        mymessageBody.str("");
	valuepass.str("");
	valueUser.str("");
	
    	auto it=allUsers.find(username);
    	if(it != allUsers.end())
    	{
        	User user=it->second;
        	if(user.password.compare(password)==0)
        	{
	    		auto findinonlinelist=onlineUsers.find(username);
	    		if(findinonlinelist == onlineUsers.end()){
				if(!(user.getOnline())){
					user.setOnline(true);
				}
				onlineUsers.insert({user.username,user});
            			flag = 1 ;
			}
		}
		it->second = user;
        }
        if(flag)
        {
		response = string("login")+'~'+string("200");
            	cout<<"Login success"<<endl;
		cout<<"Number of online users: "<<onlineUsers.size();
        }
    	else
    	{
		response = string("login")+'~'+string("500");
        	cout<<"Login failed"<<endl;
    	}
        write(sockfd,response.c_str(),strlen(response.c_str())+1);
	message.clear();
	message.str("");
    }
    else if(messageBody.compare("register")==0)
    {
	int flag = 0;
	string mystring,mystringuser,mystringpass,username,password,response,bodyMsg; 
        getline(message,bodyMsg,'~');
	stringstream mymessageBody(bodyMsg);
	getline(mymessageBody,mystringuser,'|');
	stringstream valueUser(mystringuser);
	getline(mymessageBody,mystringpass,'|');
	stringstream valuepass(mystringpass);
	getline(valueUser,mystring,':'); 
        if(mystring.compare("username")==0)
        {
            getline(valueUser,username,':');
        }
	getline(valuepass,mystring,':');
	
        if(mystring.compare("password")==0)
        {
            getline(valuepass,password,':');
        }
        mymessageBody.clear();
	valuepass.clear();
	valueUser.clear();
	mymessageBody.str("");
	valuepass.str("");
	valueUser.str("");
	for(auto it=allUsers.begin();it!=allUsers.end();it++){
		if(username == it->first)
			flag=1;
	}
	if(!flag)
        {
        	cout<<"Registration Successful"<<endl;
        	User user(username,password);
        	allUsers.insert({user.username,user});
            	response=string("register")+'~'+string("200");
        }
        else
        {
            	response=string("register")+'~'+string("500");
        	cout<<"Registration Failed"<<endl;
        }
        write(sockfd,response.c_str(),strlen(response.c_str())+1);
	message.clear();
	message.str("");
    }
    else if(messageBody.compare("location")==0)
    {
	string bodyOfMsg,firstPart,secondPart,usernametag,username,porttag,port;
	getline(message,bodyOfMsg,'~');
	stringstream mymessage(bodyOfMsg);
	getline(mymessage,firstPart,'|');
        getline(mymessage,secondPart,'|');
        stringstream mypartfirst(firstPart);
	
	getline(mypartfirst,usernametag,':');
	if(usernametag.compare("username")!=0){
		cout<<"Error: In fetching the username location"<<endl;
	}
	else{
		getline(mypartfirst,username,':');
	}

	stringstream mypartsecond(secondPart);
	getline(mypartsecond,porttag,':');
	if(porttag.compare("port")!=0){
		cout<<"Error: In fetching the port location"<<endl;
	}
	else{
		getline(mypartsecond,port,':');
	}
	mymessage.clear();
	mymessage.str("");
	mypartfirst.clear();
	mypartfirst.str("");
	mypartsecond.clear();
	mypartsecond.str("");
	string myipaddress;
	myipaddress.clear();
	myipaddress=getPeerAddressFromSock(sockfd);
	string response;
	response.clear();
	response=string("location")+'~'+"username"+':'+username+'|'+"port"+':'+port+'|'+"ip"+':'+myipaddress;
	auto it = allUsers.find(username);
	if(it != allUsers.end()){
		User currentuser;
		currentuser=it->second;
		if((currentuser.username.compare(username))==0){
			currentuser.setListeningPort(stoi(port));
			currentuser.setAddress(myipaddress);
			currentuser.setSockfd(sockfd);
		}
		auto myitr = currentuser.inviteList.begin();
		while(myitr != currentuser.inviteList.end()){
			auto findfriend = onlineUsers.find(*myitr);
			if(findfriend == onlineUsers.end()){
				myitr = currentuser.inviteList.erase(myitr);
				continue;
			}
			myitr++;
		}
		for(auto frienditr = currentuser.friendList.begin(); frienditr != currentuser.friendList.end(); frienditr++){
			auto findfriend = onlineUsers.find(*frienditr);
			if(findfriend != onlineUsers.end()){//Searching all the friend who are currently online
				currentuser.addOnlineFriendlist(*frienditr);
				User senduser;
				senduser = findfriend->second;
				int friendsocket =  senduser.getSockfd();
				write(friendsocket,response.c_str(),strlen(response.c_str())+1);
			}
		}
		it->second = currentuser;
		auto iter = onlineUsers.find(username);
		if(iter != onlineUsers.end()){
			iter->second=currentuser;	
		}
		for(auto myiter = currentuser.friendList.begin(); myiter != currentuser.friendList.end(); myiter++){
			auto findfriend = onlineUsers.find(*myiter);
			if(findfriend != onlineUsers.end()){//Searching all the friend who are currently online
				User myfriend=findfriend->second;
				string myresponse;
				myresponse.clear();
				string myusername = myfriend.getUsername();
				int myport = myfriend.getListeningPort();
				string myaddress = myfriend.getAddress();
				stringstream myportstream;
				myportstream.clear();
				myportstream.str("");
				myportstream<<myport;
				myresponse=string("location")+'~'+"username"+':'+myusername+'|'+"port"+':'+myportstream.str()+'|'+"ip"+':'+myaddress;
				write(sockfd,myresponse.c_str(),strlen(myresponse.c_str())+1);
			}
		}
	}
	message.clear();
	message.str("");
    }
    else if((messageBody.compare("invite")==0)||(messageBody.compare("accept_invite")==0))
    {
	cout<<"In invite accpt"<<endl;
	string bodyOfMsg,firstPart,secondPart,thirdPart,toUserTag,toUser,fromUser,fromUserTag,sendMessageTag,sendMessage;	
	getline(message,bodyOfMsg,'~');
	stringstream msgStream(bodyOfMsg);
	getline(msgStream,firstPart,'|');
	getline(msgStream,secondPart,'|');
	getline(msgStream,thirdPart,'|');
	stringstream firstPartSream(firstPart);
	stringstream secondPartSream(secondPart);
	stringstream thirdPartSream(thirdPart);
	getline(firstPartSream,fromUserTag,':');
	getline(firstPartSream,fromUser,':');
	getline(secondPartSream,toUserTag,':');
	getline(secondPartSream,toUser,':');
	getline(thirdPartSream,sendMessageTag,':');
	getline(thirdPartSream,sendMessage,':');
	msgStream.clear();
	msgStream.str("");
	firstPartSream.clear();
	firstPartSream.str("");
	secondPartSream.clear();
	secondPartSream.str("");
	thirdPartSream.clear();
	thirdPartSream.str("");
	string response;
	response.clear();
	if(messageBody.compare("invite")==0){
		response=string("invite")+'~'+string("fromuser")+':'+fromUser+'|'+string("touser")+':'+toUser+'|'+"message"+':'+sendMessage;
	}
	else if(messageBody.compare("accept_invite")==0){
		response=string("accept_invite")+'~'+string("fromuser")+':'+fromUser+'|'+string("touser")+':'+toUser+'|'+"message"+':'+sendMessage;

	}
	cout<<"Resp: "<<response<<endl;
	auto findfriend = onlineUsers.find(toUser);
	if(findfriend == onlineUsers.end()){//Searching all the friend who are currently online
		cout<<"The invited person is not online"<<endl;
	}else{
		if(messageBody.compare("invite")==0){
			User senduser;
			senduser = findfriend->second;
			int friendsocket =  senduser.getSockfd();
			write(friendsocket,response.c_str(),strlen(response.c_str())+1);
		}
		else if(messageBody.compare("accept_invite")==0){
			string inviterResponse, inviteeResponse;
			int inviterSocket, inviteeSocket;
			auto it=onlineUsers.find(toUser);
    			if(it!=onlineUsers.end())
    			{
        			User inviter=it->second;
        			inviter.friendList.push_back(fromUser);
				inviter.addOnlineFriendlist(fromUser);
				inviter.addInvitelist(fromUser);
        			it->second=inviter;
				inviterSocket = inviter.sockfd;
				string myusername = inviter.username;
				string myaddress = inviter.address;
				int myport =  inviter.listeningPort;
				stringstream myportstream;
				myportstream.clear();
				myportstream.str("");
				myportstream<<myport;
				inviterResponse=string("location")+'~'+"username"+':'+myusername+'|'+"port"+':'+myportstream.str()+'|'+"ip"+':'+myaddress;
				myportstream.clear();
				myportstream.str("");
        			write(inviter.sockfd,response.c_str(),strlen(response.c_str())+1);
			}	
        		auto it_1=allUsers.find(toUser);
			if(it_1 != allUsers.end()){
        			User inviter=it_1->second;
        			inviter.friendList.push_back(fromUser);
				inviter.addOnlineFriendlist(fromUser);
				inviter.addInvitelist(fromUser);
        			it_1->second=inviter;
			
			} 
        		auto itr=onlineUsers.find(fromUser);
			if(itr != onlineUsers.end()){
        			User invitee=itr->second;
        			invitee.friendList.push_back(toUser);
				invitee.addOnlineFriendlist(toUser);
				invitee.addInvitelist(toUser);
				inviteeSocket = invitee.sockfd;
        			itr->second=invitee;
				string myusername = invitee.username;
				string myaddress = invitee.address;
				int myport =  invitee.listeningPort;
				stringstream myportstream;
				myportstream.clear();
				myportstream.str("");
				myportstream<<myport;
				inviteeResponse=string("location")+'~'+"username"+':'+myusername+'|'+"port"+':'+myportstream.str()+'|'+"ip"+':'+myaddress;
				myportstream.clear();
				myportstream.str("");
			}

        		auto itr_1=allUsers.find(fromUser);
			if(itr_1 != allUsers.end()){
        			User inviteeUser=itr_1->second;
        			inviteeUser.friendList.push_back(toUser);
				inviteeUser.addOnlineFriendlist(toUser);
				inviteeUser.addInvitelist(toUser);
        			itr_1->second=inviteeUser;
			}	
			cout<<"Inviter respon"<<inviterResponse<<endl;
			cout<<"Invitee respon"<<inviteeResponse<<endl;
        		write(inviteeSocket,inviterResponse.c_str(),strlen(inviterResponse.c_str())+1);
        		write(inviterSocket,inviteeResponse.c_str(),strlen(inviteeResponse.c_str())+1);
    			}
		
			message.clear();
			message.str("");
		}
	
	message.clear();
	message.str("");
    }
    else if(messageBody.compare("message")==0)
    {	
	string bodyOfMsg,firstPart,secondPart,thirdPart,toUserTag,toUser,fromUser,fromUserTag,sendMessageTag,sendMessage;	
	getline(message,bodyOfMsg,'~');
	stringstream msgStream(bodyOfMsg);
	getline(msgStream,firstPart,'|');
	getline(msgStream,secondPart,'|');
	getline(msgStream,thirdPart,'|');
	stringstream firstPartSream(firstPart);
	stringstream secondPartSream(secondPart);
	stringstream thirdPartSream(thirdPart);
	getline(firstPartSream,fromUserTag,':');
	getline(firstPartSream,fromUser,':');
	getline(secondPartSream,toUserTag,':');
	getline(secondPartSream,toUser,':');
	getline(thirdPartSream,sendMessageTag,':');
	getline(thirdPartSream,sendMessage,':');
	msgStream.clear();
	msgStream.str("");
	firstPartSream.clear();
	firstPartSream.str("");
	secondPartSream.clear();
	secondPartSream.str("");
	thirdPartSream.clear();
	thirdPartSream.str("");
	string response;
	response.clear();
			auto it=onlineUsers.find(toUser);
    			if(it!=onlineUsers.end())
    			{
        			User receiver=it->second;
				auto findIt = find (receiver.inviteList.begin(), receiver.inviteList.end(), fromUser);
				if(findIt != receiver.inviteList.end()){
				int receiverSocket = receiver.sockfd;
				string receiverResponse;
				receiverResponse=string("mesg")+'~'+string("username")+':'+fromUser+'|'+string("message")+':'+sendMessage;
        			write(receiverSocket,receiverResponse.c_str(),strlen(receiverResponse.c_str())+1);
			}
			}	
	
        message.clear();
	message.str("");
   }
    else if(messageBody.compare("logout")==0)
    {	
	string mymessageBody;
        getline(message,mymessageBody,'~');
	string username,usernametag;
	stringstream firstPart(mymessageBody);
	getline(firstPart,usernametag,':');
	if(usernametag.compare("username")!=0){
		cout<<"Error is fetching the username for logout"<<endl;
	}
	else{
		getline(firstPart,username,':');	    
	}
	firstPart.clear();
	firstPart.str("");
	auto iter = onlineUsers.find(username);
	if(iter == onlineUsers.end()){
		cout<<"Error: User not found while loging out"<<endl;
	}else{
		User myuser;
		myuser =  iter->second;
		myuser.userLogout();
		onlineUsers.erase(iter);
		auto myiter =  allUsers.find(username);
		if(myiter!=allUsers.end()){
			myiter->second=myuser;
		}
	}
	cout<<"Number of users currently logged in is :"<<onlineUsers.size()<<endl;	 
        message.clear();
	message.str("");
	return true;
   }
   return false;
}

static void signalHandler(int signo)
{
    	cout<<endl<<"SIGINT"<<endl;
	ofstream userInfoFile ("user_info_file");
    	User user;
    	if (userInfoFile.is_open())
    	{
        	auto it=allUsers.begin();
        	while(it!=allUsers.end())
        	{
            		user=it->second;
            		userInfoFile<<user.username<<"|"<<user.password<<"|";
            		auto itr=user.friendList.begin();
            		while(itr!=user.friendList.end())
            		{
                		userInfoFile<<*itr;
                		++itr;
                		if(itr!=user.friendList.end())
                		{
                    			userInfoFile<<";";
                		}
            		}
            		userInfoFile<<"\n";
            		++it;
        	}
	}
        userInfoFile.close();
    	close(sockfd);
   	exit(0);
}
string getPeerAddressFromSock(int sockfd)
{
    string peerAddress;
    struct sockaddr_in addr;
    socklen_t len;
    memset(&addr, 0, sizeof(addr));
    len=sizeof(addr);
    getpeername( sockfd,(struct sockaddr *) &addr,&len);
    peerAddress=inet_ntoa(addr.sin_addr);
    cout<<"Peer address from socket "<<peerAddress<<endl;
    return peerAddress;

}

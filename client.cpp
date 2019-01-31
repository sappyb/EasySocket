#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <regex>
#include <iterator>
#include <map>
/*************Macro Definitions*************/
using namespace std;

static void sig_int(int);
vector<string> read_File(fstream&);
vector<string> parse_String(string line,char delimiter);
int connectToRemoteMachine(const char *ip, int port,bool exitIferror);
int serverPort = 5100;
void parseServerMessage(int sockfd, char *buf);
void *processServerConection(void *arg);
void readFromSocket(int sockfd);
void *acceptPeerConnection(void *arg);
void startReadThreadForSocket(int sockfd);
vector<string> parseBufferstring(string bufferString);
void logoutFromServer(bool showpromt);
void *process_connection(void *arg);
string username="";
bool checkStringForMessage(string bufferString, int choice);
void printOnlineFriend();
int serverSockfd;
bool loginflag=false;
string originalUser="";
vector<int> peers;
set <string> inviterUsers;


class UserFriend
{
public:
    int sockfd=-1;
    int listeningPort=-1;
    string username;
    string address;
    UserFriend()
    {
		listeningPort=-1;
		sockfd=-1;
		username="";
		address="";
    }

    UserFriend(string friendUsername,string friendIp,int friendPort)
    {
	username=friendUsername;
        address=friendIp;
        listeningPort=friendPort;
    }

    void setSockfd(int socket){
		sockfd = socket;
	}
	
	void setListeningPort(int port){
		listeningPort = port;
	}
	void setUsername(string friendUsername){
		username = friendUsername;
	}
	void setAddress(string friendIp){
		address = friendIp;
	}
	
	int getSockfd(){
		return(sockfd);
	}
	
	int getListeningPort(){
		return(listeningPort);
	}
	string getUsername(){
		return(username);
	}
	string getAddress(){
		return(address);
	}
};

map<string,UserFriend> mapOfFriends;
map<string,UserFriend> onlineFriends;

int main(int argc, char * argv[])
{
    	onlineFriends.clear();
	int servport,*sock_ptr;

	string bufferString,password,payloadString,servhost;
	pthread_t tid;
	fstream userFileFd;
	struct sigaction abc;
	abc.sa_handler = sig_int;
	sigemptyset(&abc.sa_mask);
	abc.sa_flags = 0;
	sigaction(SIGINT, &abc, NULL);

	if (argc < 2)
	{
		printf("Please provide client configuration file\n");
		exit(0);
	}
	vector<string> clientConfigFileLines;
	fstream clientFileFd;
	clientFileFd.open(argv[1],ios::in);
	if(!clientFileFd){//Checking if the file containing the User Information is opened properly.
			cout<<"Error: Error in openinf the file";
	}else{
	
			clientConfigFileLines = read_File(clientFileFd);
	}
	clientFileFd.close();
	vector<string>::iterator it;
	for(it=clientConfigFileLines.begin();it != clientConfigFileLines.end(); it++){
		vector<string> mystring;
		mystring = parse_String(*it,':');
		if((mystring.at(0).compare("Serverhost"))==0){
			servhost=mystring.at(1);
		}
		if((mystring.at(0).compare("Port"))==0){
			servport=stoi(mystring.at(1));
			serverPort=servport;
		}
	}
	if(argc == 3){
		serverPort=stoi(argv[2]);
	}
	serverSockfd=connectToRemoteMachine(servhost.c_str(),servport,true);
	sock_ptr = (int *)malloc(sizeof(int));
	*sock_ptr = serverSockfd;
	cout<<"For registration press r and then press enter"<<endl;
	cout<<"For login press l and then press enter"<<endl;
	pthread_create(&tid, NULL, &processServerConection, (void*)sock_ptr);
	
	while(getline(cin,bufferString)){
		if((bufferString.compare("r")==0)||(bufferString.compare("l")==0)){
			int flag=0;
			cout<<"Please enter username and password"<<endl;
			cout<<"Username and password should be one word"<<endl;
			string password;
			cout<<"Username:"<<endl;
			getline(cin,username);
			cout<<"Password:"<<endl;
			cin>>password;
			string messagepayload;
			messagepayload.clear();
			if((bufferString.compare("r")==0)){
				messagepayload = string("register")+'~'+string("username")+':'+username+'|'+string("password")+':'+password;
			}
			else if((bufferString.compare("l")==0)){
				if(loginflag){
					flag=1;
				}
				messagepayload = string("login")+'~'+string("username")+':'+username+'|'+string("password")+':'+password;
				
			}
			if((bufferString.compare("r")==0)||((bufferString.compare("l")==0)&&(!flag))){
				write(serverSockfd, messagepayload.c_str(), strlen(messagepayload.c_str())+1); 
			}
		}
		else if((bufferString.compare("logout")==0)||(bufferString.compare("exit")==0)){
			string response;
			response.clear();	
				for(auto itr = peers.begin(); itr != peers.end(); itr++)
				{
					int fd;
					fd = *itr;
					close(fd);
				}
			onlineFriends.clear();
			response=string("logout")+'~'+string("username")+':'+originalUser;
			write(serverSockfd,response.c_str(),strlen(response.c_str())+1);
			loginflag=false;
    			if(bufferString.compare("logout"))
    			{
				cout<<"'r' for Registration"<<endl;
    				cout<<"'l' for login"<<endl;
      
    			}
		}
		else{
			if(loginflag){
				if(checkStringForMessage(bufferString,1))
				{	
					vector<string> input;
					string type;
					string forUser;
					string message;
					message="";
					input = parseBufferstring(bufferString);
					if(input.at(0)!=""){
						type = input.at(0);
					}
					if(input.at(1)==""){
						forUser="";
						cout<<"Please kindly enter the username format is m username message"<<endl;
						continue;
					}
					else{
						forUser=input.at(1);
					}
					for(unsigned int i=2; i < input.size(); i++){
						message=message+input.at(i)+' ';
					}
					cout<<"Type :"<<type<<" For user: "<<forUser<<" Message: "<<message<<endl;
					auto it=onlineFriends.find(forUser);
					if(it == onlineFriends.end()){
						cout<<"The friend of the user is not online currently"<<endl;
					}
					else
					{
						UserFriend User=it->second;
						int socket=User.getSockfd();
						cout<<User.listeningPort<<endl;
						string response;
						response=string("message")+'~'+string("fromuser")+':'+originalUser+'|'+string("touser")+':'+forUser+'|'+string("message")+':'+message;
						cout<<response<<endl;
						write(serverSockfd,response.c_str(),strlen(response.c_str())+1);
					}
				}
				else if(checkStringForMessage(bufferString,2))
				{
					vector<string> input;
					string type;
					string forUser;
					string message;
					message = "";
					input = parseBufferstring(bufferString);
					if(input.at(0)!=""){
						type = input.at(0);
					}
					if(input.at(1)==""){
						forUser="";
						cout<<"Please kindly enter the username format is i username message"<<endl;
						continue;
					}
					else{
						forUser=input.at(1);
					}
					for(unsigned int i=2; i < input.size(); i++){
						message=message+input.at(i)+' ';
					}
					cout<<"Type :"<<type<<" For user: "<<forUser<<" Message: "<<message<<endl;
					string response;
					response.clear();
					response = string("invite")+'~'+string("fromuser")+':'+username+'|'+string("touser")+':'+forUser+'|'+string("message")+':'+message;
					cout<<response<<endl;
					write(serverSockfd, response.c_str(), strlen(response.c_str())+1);
					response.clear();

				}
				else if(checkStringForMessage(bufferString,3))
				{
					cout<<"In invite accept"<<endl;
					vector<string> input;
					string type;
					string forUser;
					string message;
					message = "";
					input = parseBufferstring(bufferString);
					if(input.at(0)!=""){
						type = input.at(0);
					}
					if(input.at(1)==""){
						forUser="";
						cout<<"Please kindly enter the username format is ia username message"<<endl;
						continue;
					}
					else{
						forUser=input.at(1);
					}
					for(unsigned int i=2; i < input.size(); i++){
						message=message+input.at(i)+' ';
					}
					auto itr=inviterUsers.find(forUser);
					if(itr==inviterUsers.end())
					{
						cout<<"No invitation found from "<<forUser<<endl;
						continue;
					}
			
					string response;
					response.clear();
					response = string("accept_invite")+'~'+string("fromuser")+':'+username+'|'+string("touser")+':'+forUser+'|'+string("message")+':'+message;
					cout<<"Response is:" <<response<<endl;
					write(serverSockfd, response.c_str(), strlen(response.c_str())+1);
					response.clear();	
				}
			}
			else{
				cout<<"Login from a user before sending message, invite or invite accept"<<endl;
			}
		}	
		bufferString.clear();
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


int connectToRemoteMachine(const char *ip, int port,bool exitIferror)
{
    int sockfd=-1;
    int rv, flag;
    struct addrinfo hints, *res, *ressave;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(ip, to_string(port).c_str() , &hints, &res)) != 0)
    {
        cout << "getaddrinfo wrong: " << gai_strerror(rv) << endl;
        if(exitIferror)
        {
            exit(1);
        }
        return sockfd;
    }
    ressave = res;
    flag = 0;
    do
    {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
        {
            flag = 1;
            break;
        }
        close(sockfd);
    }
    while ((res = res->ai_next) != NULL);
    freeaddrinfo(ressave);
    if (flag == 0)
    {
        fprintf(stderr, "cannot connect\n");
        if(exitIferror)
        {
            exit(1);
        }
    }
    return sockfd;
}

void *processServerConection(void *arg)
{
    int n;
    int sockfd;
    char buf[4096];
    sockfd = *((int *)arg);
    free(arg);
    pthread_detach(pthread_self());
    while (1)
    {
        n = read(sockfd, buf, 4096);
        if (n == 0)
        {
            printf("Server Crashed!!!\n");
            close(sockfd);
            exit(0);
        }
        parseServerMessage(sockfd, buf);
    }
}

void parseServerMessage(int sockfd, char *buf){
 	string sbuf(buf);
	stringstream message(sbuf);
	string messageBody,response;
	getline(message,messageBody,'~');

	if(messageBody.compare("register")==0)
	{
		
		getline(message,messageBody,'~');
		if(messageBody.compare("200")==0)
		{
				cout<<"Registration successful"<<endl;

		}
		else
		{
				cout<<"Registration failed. Please try again"<<endl;
		}

	}
	else if(messageBody.compare("login")==0)
	{	
		getline(message,messageBody,'~');
		if(messageBody.compare("200")==0)
		{
			stringstream myport;
			myport<<serverPort;	
			loginflag=true;
			originalUser = username;
			string response;
			response.clear();
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"***********************************************Login successful***************************************************"<<endl;
			cout<<"**************************************'m 'username' 'message' to send message*************************************"<<endl;
			cout<<"*************************************'i' 'username' 'message' to invite to chat***********************************"<<endl;
			cout<<"************************************'ia 'username' 'message' to accept invitation*********************************"<<endl;
			cout<<"************************************'logout' 'username'  to logout from the server********************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"******************************************************************************************************************"<<endl;
			cout<<"||||||||||||||||||||||||N.B.--The client will terminate with SIGNAL if wrong input format is given||||||||||||||||"<<endl;
			int serv_sockfd;
			struct sockaddr_in serv_addr;
			serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
			bzero((void*)&serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			serv_addr.sin_port = htons(serverPort);
			bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
			listen(serv_sockfd, 5);
			int * sock_ptr;
			sock_ptr = (int *)malloc(sizeof(int));
			*sock_ptr = serv_sockfd;
			response = string("location")+'~'+string("username")+':'+username+'|'+string("port")+':'+(myport.str());
			myport.clear();
			myport.str("");
			write(sockfd,response.c_str(),strlen(response.c_str())+1);
		}
		else
		{
			cout<<"Login failed"<<endl;
		}
    }
	else if(messageBody.compare("location")==0)
    {
		getline(message,messageBody,'~');
		stringstream mymessagestream(messageBody);
		string firstpart, secondpart, thirdpart;
		getline(mymessagestream,firstpart,'|');
		getline(mymessagestream,secondpart,'|');
		getline(mymessagestream,thirdpart,'|');
		stringstream firstPartStream(firstpart);
		stringstream secondPartStream(secondpart);
		stringstream thirdPartStream(thirdpart);
		string usernameTag,myusername,ipaddressTag,myipaddress,portTag,myport;
		getline(firstPartStream,usernameTag,':');
		getline(secondPartStream,portTag,':');
		getline(thirdPartStream,ipaddressTag,':');
        	if(usernameTag.compare("username")==0)
        	{
			getline(firstPartStream,myusername,':');
        	}	
        	if(ipaddressTag.compare("ip")==0)
        	{
			getline(thirdPartStream,myipaddress,':');
        	}
        	if(portTag.compare("port")==0)
        	{
			getline(secondPartStream,myport,':');
        	}
		cout<<"Friend:"<<myusername<<" is online"<<endl;
		mymessagestream.clear();
		firstPartStream.clear();
		secondPartStream.clear();
		thirdPartStream.clear();
		mymessagestream.str("");
		firstPartStream.str("");
		secondPartStream.str("");
		thirdPartStream.str("");
		UserFriend onlineFriend(myusername,myipaddress,stoi(myport));
		onlineFriends.insert({myusername,onlineFriend});
		
    }
    else if(messageBody.compare("invite")==0)
    {
        	getline(message,messageBody,'~');
		stringstream mymessagestream(messageBody);
		string firstpart,secondpart,thirdpart,fromUserTag,fromuser,toUserTag,touser,messageTag,mymessage;
		getline(mymessagestream,firstpart,'|');
		getline(mymessagestream,secondpart,'|');
		getline(mymessagestream,thirdpart,'|');
		stringstream firstPartStream(firstpart);
		stringstream secondPartStream(secondpart);
		stringstream thirdPartStream(thirdpart);
		getline(firstPartStream,fromUserTag,':');
		getline(secondPartStream,toUserTag,':');
		getline(thirdPartStream,messageTag,':');
		if(fromUserTag.compare("fromuser")==0)
		{
		getline(firstPartStream,fromuser,':');
		}
		if(toUserTag.compare("toUserTag")==0)
		{
		getline(secondPartStream,touser,':');
		}
		if(messageTag.compare("message")==0)
		{
		getline(thirdPartStream,mymessage,':');
		}

		cout<<"You have received an invitation from "<<fromuser<<endl;
		cout<<"Message is : "<<mymessage<<endl;

		auto it=inviterUsers.find(fromuser);
		if(it==inviterUsers.end())
		{
			inviterUsers.insert(fromuser);
		}
		mymessagestream.clear();
		mymessagestream.str("");
		firstPartStream.clear();
		firstPartStream.str("");
		secondPartStream.clear();
		secondPartStream.str("");
		thirdPartStream.clear();
		thirdPartStream.str("");
			
	}
    else if(messageBody.compare("mesg")==0)
    {
        	getline(message,messageBody,'~');
		stringstream mymessagestream(messageBody);
		string firstpart,secondpart,fromUserTag,fromuser,messageTag,mymessage;
		getline(mymessagestream,firstpart,'|');
		getline(mymessagestream,secondpart,'|');
		stringstream firstPartStream(firstpart);
		stringstream secondPartStream(secondpart);
		getline(firstPartStream,fromUserTag,':');
		getline(secondPartStream,messageTag,':');
		if(fromUserTag.compare("fromuser")==0)
		{
		getline(firstPartStream,fromuser,':');
		}
		if(messageTag.compare("message")==0)
		{
		getline(secondPartStream,mymessage,':');
		}

		cout<<"Message from"<<fromuser<<" is : "<<mymessage<<endl;

		mymessagestream.clear();
		mymessagestream.str("");
		firstPartStream.clear();
		firstPartStream.str("");
		secondPartStream.clear();
		secondPartStream.str("");
			
	}
	
	message.clear();
	message.str("");
}


static void sig_int(int signo){
		string response;
		response.clear();	
    	for(auto itr = peers.begin(); itr != peers.end(); itr++)
    	{
       		int fd;
       		fd = *itr;
       		close(fd);
    	}
		onlineFriends.clear();
		printOnlineFriend();
    	response=string("logout")+'~'+string("username")+':'+username;
		write(serverSockfd,response.c_str(),strlen(response.c_str())+1);
    	loginflag=false;
    	close(serverSockfd);
    	exit(0);
}
             
bool checkStringForMessage(string bufferString, int choice){
	if(choice == 1){
		if(bufferString[0]=='m'){
			return true;
		}
	}
	else if(choice == 2){
		if((bufferString[0]=='i')&&(bufferString[1]!='a')){
			return true;
		}
	}
	else if(choice == 3){
		if((bufferString[0]=='i')&&(bufferString[1]=='a')){
			return true;
		}
	}
	return false;
}

vector<string> parseBufferstring(string bufferString){
		stringstream mystream;
		vector<string> mytokens;
		mystream.clear();
		mystream.str(bufferString);
		string temp;
		while(getline(mystream,temp,' ')){
			mytokens.push_back(temp);
			temp.clear();
		}
		mystream.clear();
		mystream.str("");
		return mytokens;
}

void printOnlineFriend(){
	for(auto it = onlineFriends.begin();it!= onlineFriends.end(); it++){
		cout<<"it->first"<<it->first<<endl;
	}
}

/*
 * File:   main.cpp
 * Author: chiasheng
 *
 * Created on March 11, 2014, 2:41 PM
 */
/**/

#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<queue>
#include<vector>
#include<sstream>
#include<string>

using namespace std;


struct clientInfo {
    pthread_t threadID; // thread's pointer
    int sockfd; // socket file descriptor
    string name; // client's alias
    string grpCreator;
};

struct PACKET {
    char command[20]; // instruction
    //char clientName[20]; // client name
    char arguments[200]; // payload
};


void *listener(void*);
void *serviceclient(void*);
void recieveeachcommand(int);
void createGroup(string creator,string message);
void confirmationValidation(string userName,string confirmation);
bool isClear(string s);
bool commandValidation(string username,string command,string message,int sockfd);

string broadcastyell="";
int socketnumber,n;
struct sockaddr_in serveradd,clientadd;

//connected clients
vector<clientInfo> clientList;
socklen_t clientlen;


//clients in a group
vector<string> invitedNameGroup;
vector<clientInfo> invitedClientGroup;
vector<clientInfo> confirmClientGroup;
vector<vector<clientInfo>> groupList;

pthread_t listenerthread;
pthread_mutex_t clientListMutex;
pthread_mutex_t grpListMutex;
pthread_mutex_t grpInvitedListMutex;
pthread_mutex_t grpConfirmListMutex;

int main(int argc, char** argv) {
    
    
    //socketnumber for socket. acceptnumber for listening number
    
    //2 socket address structure one for server one for client
    //createGroup("panda", "bear bird\n");
    //    char message[1000];
    int portnumber;
    printf("=== welcome to the chat server!!===\n");
    printf("Server prompts for the Service port to use\n");
    cin>>portnumber;
    
    
    //create socket of domain- Ip address, stream based TCP , protocol undefine.
    socketnumber=socket(AF_INET,SOCK_STREAM,0);
    
    //a valid descriptor is always positive.
    if(socketnumber<0)
        printf("Failed creating socket\n");
    
    //initialise the server address struct to zero
    bzero(&serveradd,sizeof(serveradd));
    
    //fill server's address family
    serveradd.sin_family = AF_INET;
    
    //server should allow any ip address
    serveradd.sin_addr.s_addr=inet_addr("127.0.0.1");
    
    //16 bitport number on which server listens
    //hton function ensure that an integer is interpretted correctly even if client differ archit
    serveradd.sin_port=htons(portnumber);
    
    //attach the server socket to a port.Only for server does not select random port
    bind(socketnumber, (struct sockaddr *)&serveradd, sizeof(struct sockaddr));
    
    //server start listening, enable the program to halt on accept calls
    //wait until a client connects , request them all in queue.number of request held pending
    listen(socketnumber,1024);
    
    printf("waiting for connection.....\n");
    
    clientlen=sizeof(clientadd);
    
    //server block on this call until a client tries to establish the connection.
    //when a connection is establish, it return a connected socket descriptor different from the one created earlier
    
    //create thread for listening client
    pthread_create(&listenerthread,NULL,&listener,NULL);
    
    while(true)
    {
        
    }
    
    
    close(socketnumber);
    return 0;
}
void* listener(void* listensocket)
{
    int acceptNumber;
    struct clientInfo info;
    do
    {
        acceptNumber=accept(socketnumber,(struct sockaddr *)&clientadd,&clientlen);
        info.sockfd=acceptNumber;
        pthread_create(&info.threadID,NULL,&serviceclient,(void*)&info);
        
    }
    while(true);
}

//this function for talking to the client
void *serviceclient(void* fd)
{
    struct clientInfo info = *(struct clientInfo *) fd;
    
    int socket = info.sockfd;
    char buffer[50]={0};
    
    recv(socket,(void *)buffer,50,0);
    string username(buffer);
    
    cout<<username<<" is connected to the server with "<<socket<< "\n"<<flush;
    info.name=username;
    pthread_mutex_lock(&clientListMutex);
    clientList.push_back(info);
    pthread_mutex_unlock(&clientListMutex);
    //-----------------edited new part-------------------
    while(1){
        struct PACKET packet;
        recv(info.sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        string instruction(packet.command);
        string load(packet.arguments);
        cout<<instruction<<endl;
        commandValidation(username,instruction,load,socket);
        
    }
    
    return NULL;
    
}

//this function for talking to the client
void *clientConfirmation(void* fd)
{
    
    struct clientInfo info = *(struct clientInfo *) fd;
    
    int socket = info.sockfd;
    string confirmationMessage,sendInstruction,acceptanceMessage,rejectionMessage;
    struct PACKET sendPacket;
    
    
    confirmationMessage="You received an invitation from "+info.grpCreator+"|y/n ?|\n";
    sendInstruction = "confirmation";
    cout<<"Socket number:"<<socket<<endl;
    //send confirmation query to client
    strcpy(sendPacket.arguments,confirmationMessage.c_str());
    strcpy(sendPacket.command,sendInstruction.c_str());
    send(socket,(void *)&sendPacket,sizeof(struct PACKET),0);
    return NULL;
}


void createGroup(string creator,string message){
    
    string grpList;
    string grpMember;
    size_t f;
    int i,j;
    pthread_t id;
    
    //add creator to group first
    grpList=message;
    pthread_mutex_lock(&grpListMutex);
    invitedNameGroup.push_back(creator);
    pthread_mutex_lock(&grpListMutex);
    
    
    f=grpList.find("\n");
    if (f!=std::string::npos)
        grpList = grpList.replace(f,1,"");
    stringstream ss(grpList);
    //parsing request message to identify invited clients's name
    do{
      
        
        getline(ss,grpMember,' '); //seperate the name with the first name and last name using space
        //f=grpMember.find("\n");
        
       // if (f!=std::string::npos)
         //   grpMember = grpMember.replace(f,1,"");
        
        f=grpList.find(grpMember);
        cout<<"::"<<grpMember<<"::"<<endl;
        grpList=grpList.replace(f,grpMember.length(),"");
        if(!isClear(grpMember))
        {
            pthread_mutex_lock(&grpListMutex);
            invitedNameGroup.push_back(grpMember);
            pthread_mutex_unlock(&grpListMutex);
        }
        ss<<grpList;
    }while(!isClear(grpList));
    
    
    
    //matching of name requested to be added into group with connected clients
    for(i=0;i<invitedNameGroup.size();i++){
        for(j=0;j<clientList.size();j++){
            cout<<"Comparing:"<<clientList.at(j).name<<"with"<<invitedNameGroup.at(i)<<endl;
            pthread_mutex_lock(&clientListMutex);
            if(invitedNameGroup.at(i).compare(clientList.at(j).name)==0){
                cout<<"match"<<endl;
                clientList.at(j).grpCreator=creator;
                invitedClientGroup.push_back(clientList.at(j));
                pthread_mutex_unlock(&clientListMutex);
                break;
            }
            else
                cout<<invitedNameGroup.at(i)<<" is not connected to the server"<<endl;
        }
    }
    
    
    //dispatch thread to send invitation to clients
    pthread_mutex_lock(&grpInvitedListMutex);
    for(i=0;i<invitedClientGroup.size();i++){
        //prevent sending invitation to creator
        if(invitedClientGroup.at(i).name.compare(creator)!=0)
            pthread_create(&id,NULL,&clientConfirmation,(void*)&invitedClientGroup.at(i));
        //add creator to confirm group list
        else{
            pthread_mutex_lock(&grpConfirmListMutex);
            confirmClientGroup.push_back(invitedClientGroup.at(i));
            pthread_mutex_unlock(&grpConfirmListMutex);
        }
    }
    pthread_mutex_unlock(&grpInvitedListMutex);
    
    
    
    
}

void confirmationValidation(string userName,string confirmation){
    
    struct PACKET confirmPacket;
    string acceptanceMessage,rejectionMessage;
    
    int i;
    
    cout<<"reply from "<<userName<<confirmation<<endl;
    
    //validate reply whether it is acceptance or rejection
    if(confirmation.compare("y\n")==0){
        cout<<"Accepted group invitation from "<<userName<<endl;
        acceptanceMessage = "Client " + userName + " accepts the group chat request\n";
        strcpy(confirmPacket.arguments,acceptanceMessage.c_str());
        
        pthread_mutex_lock(&clientListMutex);
        //find confirm client info and push into confirm list
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==0){
                pthread_mutex_lock(&grpConfirmListMutex);
                confirmClientGroup.push_back(clientList.at(i));
                pthread_mutex_unlock(&grpConfirmListMutex);
                break;
            }
        }
        pthread_mutex_unlock(&clientListMutex);
        
    }
    else{
        cout<<"Declined group invitation from "<<userName<<endl;
        rejectionMessage = "Client " + userName + " rejects the group chat request\n";
        strcpy(confirmPacket.arguments,rejectionMessage.c_str());
    }
    
    //send notification of acceptance or rejection to every invited member
    pthread_mutex_lock(&grpInvitedListMutex);
    for(i=0;i<invitedClientGroup.size();i++){
        //prevent sending confirmation message to ownself
        if(invitedClientGroup.at(i).name.compare(userName)!=0)
            send(invitedClientGroup.at(i).sockfd,(void *)&confirmPacket,sizeof(struct PACKET),0);
    }
    pthread_mutex_unlock(&grpInvitedListMutex);
    
}


bool commandValidation(string userName,string command,string message,int clientSockFd)
{
    int match =0;
    int sockfd=-1;
    size_t f;
    string receiverName;
    string rawMessage;
    string finalMessage;
    struct PACKET sendPacket;
    
    int i,j;
    int groupFlag=0;
    cout<<"from client:"<<command<<" "<<message<<endl;
    //operation 1 - talk
    if(command.compare("talk")==match){
        
        //seperate receiver name from the raw message
        rawMessage= message;
        stringstream ss(rawMessage);
        getline(ss,receiverName, ' ');
        f=rawMessage.find(receiverName);
        rawMessage=rawMessage.replace(f,receiverName.length(),"");
        
        cout<<"talk to "<<receiverName<<" : "<<rawMessage<<endl;
        
        //retrieving client sockfd
        pthread_mutex_lock(&clientListMutex);
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(receiverName)==0)
                sockfd=clientList.at(i).sockfd;
        }
        pthread_mutex_unlock(&clientListMutex);
        
        finalMessage=userName +" says:";
        finalMessage.append(rawMessage);
        strcpy(sendPacket.arguments,finalMessage.c_str());
        
        if(sockfd!=-1){
            send(sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
            cout<<"sent"<<endl;
        }
        else
            cout<<"no matching name"<<endl;
    }
    
    //operation 2 - yell
    else if(command.compare("yell")==match){
        
        cout<<"yell";
        finalMessage =userName+" says:"+message;
        strcpy(sendPacket.arguments,finalMessage.c_str());
        pthread_mutex_lock(&clientListMutex);
        
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==match);
            else{
                    send(clientList.at(i).sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
            }
            
        }
        pthread_mutex_unlock(&clientListMutex);
        
    }
    
    //operation 2 - show
    else if(command.compare("show\n")==match){
        
       finalMessage = "====User(s) Online====\n";
       
        pthread_mutex_lock(&clientListMutex);
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==match)
                finalMessage.append(clientList.at(i).name+" (you)\n");
            else
                finalMessage.append(clientList.at(i).name+"\n");

        }
        pthread_mutex_unlock(&clientListMutex);
        
        strcpy(sendPacket.arguments,finalMessage.c_str());
        send(clientSockFd,(void *)&sendPacket,sizeof(struct PACKET), 0);


      
    }
    
    //operation 3 - creategroup
    else if(command.compare("creategroup")==match){
        
        cout<<"creategroup"<<endl;
        createGroup(userName,message);
    }
    
     else if(command.compare("discuss")==match){
     
     pthread_mutex_lock(&grpConfirmListMutex);
         for(i=0;i<confirmClientGroup.size();i++){
             if(confirmClientGroup.at(i).name.compare(userName)==0)
                 groupFlag=1;
         }
     pthread_mutex_unlock(&grpConfirmListMutex);
     //compose message only client is in the group
     if(groupFlag==1){
     finalMessage = userName + " says: " + message;
     strcpy(sendPacket.arguments,finalMessage.c_str());
     strcpy(sendPacket.arguments,finalMessage.c_str());
     
     //retrieve group member sockfd
     pthread_mutex_lock(&grpConfirmListMutex);
     for(i=0;i<confirmClientGroup.size();i++){
     
     //do nothing when it is sender itself
     if(confirmClientGroup.at(i).name.compare(userName)==0);
     //send message to all group member
     else{
     send(confirmClientGroup.at(i).sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
     }
     }
     pthread_mutex_unlock(&grpConfirmListMutex);
     }
     else{
         cout<<"You are not in a group chat"<<endl;
     }
         
     }
     //operation - exit
     else if(command.compare("exit\n")==match){
     cout<<"Finally exit"<<endl;
     
     //retrieving client sockfd to give exit notification
     pthread_mutex_lock(&clientListMutex);
     for(i=0;i<clientList.size();i++){
     if(clientList.at(i).name.compare(userName)==0)
     finalMessage="You have logged out\n";
     
     else
     finalMessage=userName + " has logged out\n";
     
     sockfd=clientList.at(i).sockfd;
     
     pthread_mutex_unlock(&clientListMutex);
     
     strcpy(sendPacket.arguments,finalMessage.c_str());
     
     if(sockfd!=-1){
     send(sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
     }
     
     }
     
     //remove client from connected client list
     pthread_mutex_lock(&clientListMutex);
     for(i=0;i<clientList.size();i++){
     if(clientList.at(i).name.compare(userName)==match){
     //client at the back of vector
     if(i==clientList.size()-1)
     clientList.pop_back();
     //delete client that left from the vector
     else{
     for(int j=i;j<clientList.size()-1;j++){
     clientList.at(j)=clientList.at(j+1);
     }
     clientList.pop_back();
     }
     
     }
     }
     pthread_mutex_unlock(&clientListMutex);
     
     //remove client from group if client is in the group
     //remove client from connected client list
     pthread_mutex_lock(&grpConfirmListMutex);
     for(i=0;i<confirmClientGroup.size();i++){
     if(confirmClientGroup.at(i).name.compare(userName)==match){
     //client at the back of vector
     if(i==confirmClientGroup.size()-1)
     confirmClientGroup.pop_back();
     //delete client that left from the vector
     else{
     for( j=i;j<confirmClientGroup.size()-1;j++){
     confirmClientGroup.at(j)=confirmClientGroup.at(j+1);
     }
     confirmClientGroup.pop_back();
     }
     
     }
     }
     pthread_mutex_unlock(&grpConfirmListMutex);
     
     
     return false;
     }
     
     //operation - leavegroup
     else if(command.compare("leavegroup\n")==match){
     cout<<"Leave group"<<endl;
     
     //retrieving client sockfd to give exit notification
     pthread_mutex_lock(&grpConfirmListMutex);
     for(i=0;i<confirmClientGroup.size();i++){
     if(confirmClientGroup.at(i).name.compare(userName)==0)
     finalMessage="You have left the group chat\n";
     
     else
     finalMessage=userName + " has left the group chat\n";
     
     sockfd=confirmClientGroup.at(i).sockfd;
     
     pthread_mutex_unlock(&grpConfirmListMutex);
     
     strcpy(sendPacket.arguments,finalMessage.c_str());
     
     if(sockfd!=-1){
     send(sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
     }
     
     }
     
     //remove client from group if client is in the group
     //remove client from connected client list
     pthread_mutex_lock(&grpConfirmListMutex);
     for(i=0;i<confirmClientGroup.size();i++){
     if(confirmClientGroup.at(i).name.compare(userName)==match){
     //client at the back of vector
     if(i==confirmClientGroup.size()-1)
     confirmClientGroup.pop_back();
     //delete client that left from the vector
     else{
     for( j=i;j<confirmClientGroup.size()-1;j++){
     confirmClientGroup.at(j)=confirmClientGroup.at(j+1);
     }
     confirmClientGroup.pop_back();
     }
     
     }
     }
     pthread_mutex_unlock(&grpConfirmListMutex);
     
     }
     
     //operation 5 - confirmation
     else
     confirmationValidation(userName,command);
    
    return true;
}

//helper function
bool isClear(string s)
{
    int i;
    for(i=0;i<s.length();i++){
        if(isalpha(s[i]))
            return false;
    }
    return true;
}




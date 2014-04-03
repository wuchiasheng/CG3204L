/*
 * File:   Server.cpp
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
    int grpNumber;//group number joined
    string grpCreator;//creator's name of the group chat joined
    int portNumber;
    
};

struct PACKET {
    char command[20]; // instruction
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
vector<vector<clientInfo> > groupList;

pthread_t listenerthread;
pthread_mutex_t clientListMutex;
pthread_mutex_t grpListMutex;
pthread_mutex_t grpNameMutex;
pthread_mutex_t grpInvitedListMutex;
pthread_mutex_t grpConfirmListMutex;

int main(int argc, char** argv) {
    
    
    //socketnumber for socket. acceptnumber for listening number
    
    //2 socket address structure one for server one for client
    //createGroup("panda", " panda bear dog\n");
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
    cout<<"chat server running on "<<"127.0.0.1:"<<portnumber<<endl;
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
        info.portNumber=ntohs(clientadd.sin_port);
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
    
    cout<<"Client "<<username<<" connected from 127.0.0.1:"<<info.portNumber<<"\n";
    info.name=username;
    info.grpNumber=-1;
    pthread_mutex_lock(&clientListMutex);
    clientList.push_back(info);
    pthread_mutex_unlock(&clientListMutex);
    //-----------------edited new part-------------------
    while(1){
        struct PACKET packet;
        recv(info.sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        string instruction(packet.command);
        string load(packet.arguments);
        //cout<<instruction<<endl;
        if(commandValidation(username,instruction,load,socket)==false)
            break;
        
    }
    
    return NULL;
    
}

//send invitation to invited clients
void *clientConfirmation(void* fd)
{
    
    struct clientInfo info = *(struct clientInfo *) fd;
    
    int socket = info.sockfd;
    string confirmationMessage,sendInstruction,acceptanceMessage,rejectionMessage;
    struct PACKET sendPacket;
    
    
    confirmationMessage="You received an invitation from "+info.grpCreator+"|y/n ?|\n";
    sendInstruction = "confirmation";
    //cout<<"Socket number:"<<socket<<endl;
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
    
    
    grpList=message;
    
    
    
    f=grpList.find("\n");
    if (f!=std::string::npos)
        grpList = grpList.replace(f,1,"");
    stringstream ss(grpList);
    //parsing request message to identify invited clients's name
    
    pthread_mutex_lock(&grpNameMutex);
    invitedNameGroup.clear();
    invitedNameGroup.push_back(creator);
    //add creator to group first
    do{
        getline(ss,grpMember,' '); //seperate the name with the first name and last name using space
        f=grpList.find(grpMember);
        //cout<<"::"<<grpMember<<"::"<<endl;
        grpList=grpList.replace(f,grpMember.length(),"");
        if(!isClear(grpMember))
        {
            
            invitedNameGroup.push_back(grpMember);
            
        }
        ss<<grpList;
    }while(!isClear(grpList));
    pthread_mutex_unlock(&grpNameMutex);
    
    cout<<"Client "<<creator<<" requests group chat of {";
    for(i=0;i<invitedNameGroup.size();i++){
        if(i!=invitedNameGroup.size()-1)
            cout<<invitedNameGroup.at(i)<<", ";
        else
            cout<<invitedNameGroup.at(i)<<"}"<<endl;
    }
    
    
    //matching of name requested to be added into group with connected clients
    pthread_mutex_lock(&clientListMutex);
    invitedClientGroup.clear();
    for(i=0;i<invitedNameGroup.size();i++){
        for(j=0;j<clientList.size();j++){
            //cout<<"Comparing:"<<clientList.at(j).name<<"with"<<invitedNameGroup.at(i)<<endl;
            
            if(invitedNameGroup.at(i).compare(clientList.at(j).name)==0){
                //cout<<"match"<<endl;
                if(clientList.at(j).grpNumber==-1){
                    clientList.at(j).grpCreator=creator;
                    invitedClientGroup.push_back(clientList.at(j));
                }
                //else
                //cout<<clientList.at(j).name<<" has joined a group"<<endl;
                break;
            }
            //else
            // cout<<invitedNameGroup.at(i)<<" is not connected to the server"<<endl;
        }
    }
    pthread_mutex_unlock(&clientListMutex);
    
    
    //add creator to confirm group list
    pthread_mutex_lock(&grpInvitedListMutex);
    pthread_mutex_lock(&clientListMutex);
    confirmClientGroup.clear();
    for(i=0;i<clientList.size();i++){
        if(clientList.at(i).name.compare(creator)==0){
            clientList.at(i).grpNumber=(int)groupList.size();
            pthread_mutex_lock(&grpConfirmListMutex);
            confirmClientGroup.push_back(clientList.at(i));
            pthread_mutex_lock(&grpListMutex);
            groupList.push_back(confirmClientGroup);
            pthread_mutex_unlock(&grpListMutex);
            pthread_mutex_unlock(&grpConfirmListMutex);
        }
    }
    pthread_mutex_unlock(&clientListMutex);
    
    //dispatch thread to send invitation to clients
    for(i=0;i<invitedClientGroup.size();i++){
        //prevent sending invitation to creator
        if(invitedClientGroup.at(i).name.compare(creator)!=0)
            pthread_create(&id,NULL,&clientConfirmation,(void*)&invitedClientGroup.at(i));
    }
    pthread_mutex_unlock(&grpInvitedListMutex);
    
    
    
    
}

void confirmationValidation(string userName,string confirmation){
    
    struct PACKET confirmPacket;
    string acceptanceMessage,rejectionMessage;
    
    int i;
    int grpNumber;
    
    //cout<<"reply from "<<userName<<confirmation<<endl;
    
    //validate reply whether it is acceptance or rejection
    if(confirmation.compare("y\n")==0){
        cout<<"Client "<<userName<<" accepts the group chat request"<<endl;
        acceptanceMessage = "Client " + userName + " accepts the group chat request\n";
        strcpy(confirmPacket.arguments,acceptanceMessage.c_str());
        
        pthread_mutex_lock(&clientListMutex);
        //find confirm client info and push into confirm list
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==0){
                pthread_mutex_lock(&grpListMutex);
                grpNumber = (int)groupList.size()-1.0;
                //cout<<"Join group number ="<<grpNumber<<endl;
                clientList.at(i).grpNumber=grpNumber;
                groupList.at(grpNumber).push_back(clientList.at(i));
                pthread_mutex_unlock(&grpListMutex);
                break;
            }
        }
        pthread_mutex_unlock(&clientListMutex);
        
    }
    else{
        cout<<"Client "<<userName<<" rejects the group chat request"<<endl;
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
    struct clientInfo client;
    
    int i,j;
    //cout<<"from client:"<<command<<" "<<message<<endl;
    //operation 1 - talk
    if(command.compare("talk")==match){
        
        //seperate receiver name from the raw message
        rawMessage= message;
        stringstream ss(rawMessage);
        getline(ss,receiverName, ' ');
        f=rawMessage.find(receiverName);
        rawMessage=rawMessage.replace(f,receiverName.length(),"");
        
        //cout<<"talk to "<<receiverName<<" : "<<rawMessage<<endl;
        
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
            //cout<<"sent"<<endl;
        }
        else
            cout<<"no matching name"<<endl;
    }
    
    //operation 2 - yell
    else if(command.compare("yell")==match){
        
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
        
        //cout<<"creategroup"<<endl;
        createGroup(userName,message);
    }
    
    else if(command.compare("discuss")==match){
        int grpNumber=0;
        pthread_mutex_lock(&clientListMutex);
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==0)
                grpNumber = clientList.at(i).grpNumber;
        }
        pthread_mutex_unlock(&clientListMutex);
        //cout<<"groupnumber:"<<grpNumber<<endl;
        //compose message only client is in the group
        if(grpNumber>=0){
            finalMessage = userName + " says: " + message;
            strcpy(sendPacket.arguments,finalMessage.c_str());
            strcpy(sendPacket.arguments,finalMessage.c_str());
            
            //retrieve group member sockfd
            pthread_mutex_lock(&grpListMutex);
            for(i=0;i<groupList.at(grpNumber).size();i++){
                //do nothing when it is sender itself
                if(groupList.at(grpNumber).at(i).name.compare(userName)==0);
                //send message to all group member
                else{
                    send(groupList.at(grpNumber).at(i).sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
                }
            }
            pthread_mutex_unlock(&grpListMutex);
        }
        else{
            // cout<<"You are not in a group chat"<<endl;
        }
        
    }
    //operation - exit
    else if(command.compare("exit\n")==match){
        //cout<<"Finally exit"<<endl;
        
        //retrieving client sockfd to give exit notification
        pthread_mutex_lock(&clientListMutex);
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==0){
                //finalMessage="You have logged out\n";
                client =clientList.at(i);
                pthread_kill(client.threadID,0);
                close(client.sockfd);
            }
            else{
                finalMessage=userName + " has logged out\n";
                
                sockfd=clientList.at(i).sockfd;
                
                pthread_mutex_unlock(&clientListMutex);
                
                strcpy(sendPacket.arguments,finalMessage.c_str());
                
                if(sockfd!=-1){
                    send(sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
                }
            }
        }
        
        cout<<"Client "<<client.name<<" exited from 127.0.0.1:"<<client.portNumber<<endl;
        
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
        pthread_mutex_lock(&grpListMutex);
        if(client.grpNumber>=0){
            for(i=0;i<groupList.at(client.grpNumber).size();i++){
                if(groupList.at(client.grpNumber).at(i).name.compare(userName)==match){
                    //client at the back of vector
                    if(i==groupList.at(client.grpNumber).size()-1)
                        groupList.at(client.grpNumber).pop_back();
                    //delete client that left from the vector
                    else{
                        for(j=i;j<groupList.at(client.grpNumber).size()-1;j++){
                            groupList.at(client.grpNumber).at(j)=groupList.at(client.grpNumber).at(j+1);
                        }
                        groupList.at(client.grpNumber).pop_back();
                    }
                    
                }
            }
            pthread_mutex_unlock(&grpListMutex);
            
            
        }
        
        return false;
    }
    //operation - leavegroup
    else if(command.compare("leavegroup\n")==match){
        cout<<"Client "<<userName<<" leaves group chat"<<endl;
        
        for(i=0;i<clientList.size();i++){
            if(clientList.at(i).name.compare(userName)==0){
                client=clientList.at(i);
                clientList.at(i).grpNumber=-1;
                break;
            }
        }
        
        if(client.grpNumber>=0){
            //retrieving client sockfd to give exit notification
            pthread_mutex_lock(&grpListMutex);
            for(i=0;i<groupList.at(client.grpNumber).size();i++){
                if(groupList.at(client.grpNumber).at(i).name.compare(userName)==0)
                    finalMessage="You have left the group chat\n";
                
                else
                    finalMessage=userName + " has left the group chat\n";
                
                sockfd=groupList.at(client.grpNumber).at(i).sockfd;
                
                pthread_mutex_unlock(&grpListMutex);
                
                strcpy(sendPacket.arguments,finalMessage.c_str());
                
                if(sockfd!=-1){
                    send(sockfd,(void *)&sendPacket,sizeof(struct PACKET), 0);
                }
                
            }
            
            //remove client from group if client is in the group
            //remove client from connected client list
            pthread_mutex_lock(&grpListMutex);
            for(i=0;i<groupList.at(client.grpNumber).size();i++){
                if(groupList.at(client.grpNumber).at(i).name.compare(userName)==match){
                    //client at the back of vector
                    if(i==groupList.at(client.grpNumber).size()-1)
                        groupList.at(client.grpNumber).pop_back();
                    //delete client that left from the vector
                    else{
                        for( j=i;j<groupList.at(client.grpNumber).size()-1;j++){
                            groupList.at(client.grpNumber).at(j)=groupList.at(client.grpNumber).at(j+1);
                        }
                        groupList.at(client.grpNumber).pop_back();
                    }
                    
                }
            }
            pthread_mutex_unlock(&grpListMutex);
            
        }
    }
    
    //operation 5 - confirmation
    else if(command.compare("confirmation")==match)
        confirmationValidation(userName,message);
    
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




//
//  client.cpp
//  cg3204
//
//  Created by Wu Chia Sheng on 21/3/14.
//  Copyright (c) 2014 ___wcs___. All rights reserved.
//

/**/


// File name – client.c
// Written and tested on Linux Fedora Core 12 VM
#include<iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <netdb.h>
#include<cstring>
#include <sstream>
#include <vector>
#include <sys/select.h>
#include <unistd.h>

#define SIZE 1024
#define STDIN 0

using namespace std;

struct PACKET {
    char command[20]; // instruction
    //char clientName[20]; // client name
    char arguments[200]; // payload
};

int portnumber;
int clientportnumber;
struct sockaddr_in clientadd;


bool isClear(string s);
void login(int sockNum);
void printhelpcommand(void);
void commandValidation(PACKET &);
void createGroup(void);


int main()
{
    //the help file and the cin input can be done here the program....
    
    // Client socket descriptor which is just integer number used to access a socket
    int socketnumber;
    struct sockaddr_in serveradd;
    
    string serverip;
    // Structure from netdb.h file used for determining host name from local host's ip address
    struct hostent *server;
    
    printf("=== Welcome to the chat client!! ===\n");
    printf("Enter chat server IP: ");
    // cin>>serverip;
    printf("Enter chat server port: ");
    // cin>>portnumber;
    //printhelpcommand();
    //commandValidation();
    
    serverip="127.0.0.1";
    portnumber =4321;
    
    // Create socket of domain - Internet (IP) address, type - Stream based (TCP) and protocol unspecified
    // since it is only useful when underlying stack allows more than one protocol and we are choosing one.
    // 0 means choose the default protocol.
    
    bzero((char *)&serveradd, sizeof(serveradd));
    
    //server = gethostbyname("127.0.0.1");
    server = gethostbyname(serverip.c_str());
    
    if(server == NULL)
    {
        printf("Failed finding server name\n");
        return -1;
    }
    
    serveradd.sin_family = AF_INET;
    
    // 16 bit port number on which server listens
    // The function htons (host to network short) ensures that an integer is
    // interpreted correctly (whether little endian or big endian) even if client and
    // server have different architectures
    
    serveradd.sin_port = htons(portnumber);
    
    //  for(int i =0;i<5;i++) //for troubleshooting the pthread
    //  {
    
    socketnumber = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socketnumber < 0)
        printf("Failed creating socket\n");
    
    if (connect(socketnumber, (struct sockaddr *)&serveradd, sizeof(serveradd)) < 0)
    {
        printf("Failed to connect to server\n");
        return -1;
    }
    else
        printf("Connected successfully \n");
    //} //for troubleshooting the pthread
    unsigned clientlen=sizeof(clientadd);
    getsockname(socketnumber,(struct sockaddr *)&clientadd,&clientlen);
    
    login(socketnumber);
    close(socketnumber);
	return 0;
}
void login(int socketnumber)
{
    
    string username;
    string commandstring;
    struct PACKET packet;
    struct PACKET recvPacket;
    char replyfromserverbuffer[50]={0};
    
    
    fd_set readfds;
    
    cout<<"Client prompts for user name to use\n";
    cout<<"Enter username \n";
    cin>>username;
    //strlcpy(&packet.clientName[0],username.c_str(),username.length());
    send(socketnumber,(void*) username.c_str(),username.length(),0);
    cout<<"client "<<username<<" 127.0.0.1 "<<clientadd.sin_port<<" is connected to the server running  on 127.0.0.1"<<":"<<portnumber<<"\n";
    cout<<"==== welcome "<<username<<" to CS3103 Chat! ==== \n";
    printhelpcommand();
    
    while (1) {
        char buf[SIZE]={0};
        
        FD_CLR(socketnumber, &readfds);
        FD_SET(socketnumber, &readfds);
        FD_SET(STDIN, &readfds);
        int i=0;
        int j=0;
        for(i=0;i<20;i++)
        {
            packet.command[i]=0;
            recvPacket.command[i]=0;
        }
        
        for(j=0;j<200;j++)
        {
            packet.arguments[j]=0;
            recvPacket.arguments[j]=0;
        }
        
        select(socketnumber+1, &readfds, NULL, NULL, NULL);
        
        if (FD_ISSET(STDIN, &readfds)) {
            i=0;
            j=0;
            read(0,buf,SIZE);
            while(buf[i]!='\0' && buf[i]!=' '){
                packet.command[j]=buf[i];
                i++;
                j++;
            }
            i++;
            j=0;
            while(buf[i]!='\0'){
                packet.arguments[j]=buf[i];
                i++;
                j++;
            }
            send(socketnumber,(void *)&packet,sizeof(struct PACKET),0);
            
        }else if (FD_ISSET(socketnumber, &readfds)) {
            recv(socketnumber,(void *)&recvPacket, sizeof(struct PACKET), 0);
            string message(recvPacket.arguments);
            string instruction(recvPacket.command);
            cout<<">>"<<message;
        }
    }
    close(socketnumber);
    
    
    
}
void printhelpcommand(void)
{
    cout<<"1. show : show all users online\n";
    cout<<"2. talk <user> <message>\n";
    cout<<"3. yell <message>\n";
    cout<<"4. creategroup <user1> <user2>... chat group\n";
    cout<<"5. discuss <message> send message to users in group chat \n";
    cout<<"6. leavegroup :leave groupchat \n";
    cout<<"7. help :display all command\n";
    cout<<"8. exit\n";
}
/*
 void commandValidation(PACKET &packet)
 {
 string inputCommand;
 string load;
 int match =0;
 int exitFlag=0;
 do{
 cin>>inputCommand;
 
 if(inputCommand.compare("talk")==match){
 packet.command="talk";
 cin>>load;
 packet.arguments.push_back(load);
 
 }
 
 
 else if(inputCommand.compare("yell")==match){
 cout<<"yell";
 }
 
 else if(inputCommand.compare("creategroup")==match){
 createGroup();
 }
 
 else if(inputCommand.compare("exit")==match){
 exitFlag=1;
 cout<<"Exit"<<endl;
 }
 }while(exitFlag!=1);
 }
 */
void createGroup(){
    
    string test;
    string test2;
    size_t f;
    vector<string> group;
    int i;
    
    getline(cin,test);
    stringstream ss(test);
    
    do{
        
        getline(ss,test2, ' '); //seperate the name with the first name and last name using space
        f=test.find(test2);
        test=test.replace(f,test2.length(),"");
        if(!isClear(test2))
        {
            group.push_back(test2);
        }
        ss<<test;
    }while(!isClear(test));
    
    for(i=0;i<group.size();i++)
    {
        cout<<i<<"."<<group.at(i)<<endl;
    }
    
}

bool isClear(string s)
{
    int i;
    for(i=0;i<s.length();i++){
        if(isalpha(s[i]))
            return false;
    }
    return true;
}


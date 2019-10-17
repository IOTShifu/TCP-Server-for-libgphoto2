#ifndef MAIN_H_
#define MAIN_H_

#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gphoto2pp/ShifuCommunicator.hpp>

#define DD_CLIENT "ShifuClient"
#define DD_SERVER "ShifuServer"

#define TCP_PORT_COMMUNICATION    5555
#define TCP_PORT_LIVE_STREAMING   4444
#define TCP_PORT_IMAGE_DOWNLOADER 6666

#define SA      struct sockaddr
#define MAX_LEN 512
#define HAVE_MSGHDR_MSG_CONTROL 1

//int createUdpSocket();
void startTCPSocketCommandServer(int port);
void * clientTCPThread(void *);

void * startTCPSocketLVSLauncher(void *param);
void startTCPSocketLiveStreamingServer(int port);
void * clientTCPThreadLiveStreaming(void *);

void * startTCPSocketImageDownloader(void *param);
void startTCPSocketImageDownloaderServer(int port);
void * clientTCPThreadImageDownloader(void *);

	
ShifuCommunicator communicator;

//void joinGroup(int s, char *group);
//void leaveGroup(int recvSock, char *group);
//void reusePort(int s);
//void setTTLvalue(int s, u_char * ttl_value);
//void setLoopback(int s, u_char loop);
//void displayDaddr(int s);

#endif //MAIN_H_



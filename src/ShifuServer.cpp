#include <gphoto2pp/ShifuServer.hpp>

using namespace std;

int main() {

	syslog(LOG_INFO, "ShifuServer starting...");
    int r = 0;

	pthread_t threadLiveStreaming;
	r = pthread_create(&threadLiveStreaming, NULL, startTCPSocketLVSLauncher, NULL);

    pthread_t threadImageDownload;
    r = pthread_create(&threadImageDownload, NULL, startTCPSocketImageDownloader, NULL);

    //Main Thread handling Command Server
    startTCPSocketCommandServer(TCP_PORT_COMMUNICATION);
	
	return 0;
}

void startTCPSocketCommandServer(int port) {
	int sockfd;
	int clientTCPSocket;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	syslog(LOG_INFO, "Starting TCP socket server on port %d", port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		syslog(LOG_ERR, "ERROR opening TCP socket: %d", sockfd);
		return;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		syslog(LOG_ERR, "ERROR on binding");
		return;
	}
	listen(sockfd, 5);

	clilen = sizeof(cli_addr);

	while (true) { 
		syslog(LOG_INFO, "Awaiting client connection on TCP: %d",port);
		
		clientTCPSocket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (clientTCPSocket < 0) {
			syslog(LOG_ERR, "Error on accept client connection %d", clientTCPSocket);
		} else {
			syslog(LOG_INFO, "Incoming client connection on TCP %d",port);

			pthread_t myThread;

			int r = pthread_create(&myThread, NULL, clientTCPThread, &clientTCPSocket);

			if (r)
				syslog(LOG_ERR, "error creating client thread on %d",port);
			// close client socket
			// close(clientTCPSocket);
			// syslog(LOG_INFO, "Client finished");
		}
	}
}

void * startTCPSocketLVSLauncher(void *param) {

    startTCPSocketLiveStreamingServer(TCP_PORT_LIVE_STREAMING);

    pthread_exit(NULL);
}

void startTCPSocketLiveStreamingServer(int port) {
	int sockfd;
	int clientTCPSocketLiveStream;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	syslog(LOG_INFO, "Starting TCP socket server on port %d", port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		syslog(LOG_ERR, "ERROR opening TCP socket: %d", sockfd);
		return;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		syslog(LOG_ERR, "ERROR on binding");
		return;
	}
	listen(sockfd, 5);

	clilen = sizeof(cli_addr);

	while (true) { 
		syslog(LOG_INFO, "Awaiting client connection on TCP: %d",port);
		
		clientTCPSocketLiveStream = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (clientTCPSocketLiveStream < 0) {
			syslog(LOG_ERR, "Error on accept client connection %d", clientTCPSocketLiveStream);
		} else {
			syslog(LOG_INFO, "Incoming client connection on TCP %d",port);

			pthread_t myThread;

			int r = pthread_create(&myThread, NULL, clientTCPThreadLiveStreaming, &clientTCPSocketLiveStream);

			if (r)
				syslog(LOG_ERR, "error creating client thread on %d",port);
			// close client socket
			// close(clientTCPSocket);
			// syslog(LOG_INFO, "Client finished");
		}
	}
}


void * startTCPSocketImageDownloader(void *param)
{
    startTCPSocketImageDownloaderServer(TCP_PORT_IMAGE_DOWNLOADER);

    pthread_exit(NULL);
}
void startTCPSocketImageDownloaderServer(int port)
{
    int sockfd;
    int clientTCPSocketImageDownloader;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    syslog(LOG_INFO, "Starting TCP socket server on port %d", port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        syslog(LOG_ERR, "ERROR opening TCP socket: %d", sockfd);
        return;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        syslog(LOG_ERR, "ERROR on binding");
        return;
    }
    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    while (true) {
        syslog(LOG_INFO, "Awaiting client connection on TCP: %d",port);

        clientTCPSocketImageDownloader = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (clientTCPSocketImageDownloader < 0) {
            syslog(LOG_ERR, "Error on accept client connection %d", clientTCPSocketImageDownloader);
        } else {
            syslog(LOG_INFO, "Incoming client connection on TCP %d",port);

            pthread_t myThread;

            int r = pthread_create(&myThread, NULL, clientTCPThreadImageDownloader, &clientTCPSocketImageDownloader);

            if (r)
                syslog(LOG_ERR, "error creating client thread on %d",port);
            // close client socket
            // close(clientTCPSocket);
            // syslog(LOG_INFO, "Client finished");
        }
    }

}


void * clientTCPThread(void * param) {

    int clientTCPSocket = *(unsigned int *)param;
	communicator.handleClientConnection(clientTCPSocket);
    syslog(LOG_INFO, "****************** Exiting Command Server Connection ***************** ");
	pthread_exit(NULL);
}

void * clientTCPThreadLiveStreaming(void * param) {

    int clientTCPSocketLiveStream = *(unsigned int *)param;
	communicator.SendLiveStreamToClient(clientTCPSocketLiveStream);

	pthread_exit(NULL);
}


void * clientTCPThreadImageDownloader(void * param)
{
    int socketToDownloadImage = *(unsigned int *)param;
    communicator.DownloadImageFromFileWrapper(socketToDownloadImage);

    pthread_exit(NULL);
}






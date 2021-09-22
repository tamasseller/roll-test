#include "Tcp.h"

#include <arpa/inet.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <string.h>
#include <unistd.h>

#include <stdexcept>

void closeNow(int sock)
{
    shutdown(sock, SHUT_WR);
    close(sock);
}

int listen(uint16_t port)
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
    if (listenFd == -1)
        throw std::runtime_error("socket creation failed...\n"); 

    int enable = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

    struct sockaddr_in servaddr; 
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port); 
  
    if((bind(listenFd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
        throw std::runtime_error("socket bind failed");

    if((listen(listenFd, 5)) != 0) 
        throw std::runtime_error("listen failed");

    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    int sockfd = accept(listenFd, (struct sockaddr *)&cli, &len);

    if(sockfd < 0) 
        throw std::runtime_error("server acccept failed");

    close(listenFd); 
    
    return sockfd;
}

int connect(const char* addr, uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if(sockfd == -1) 
        throw std::runtime_error("socket creation failed"); 

    struct sockaddr_in servaddr; 
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(port); 
  
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
        throw std::runtime_error("connection with the server failed");
  
    return sockfd; 
}

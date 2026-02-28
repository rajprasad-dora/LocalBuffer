#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "contracts/fileSocketOperations.hpp"
#include <map>
#include <string>

using namespace std;
 
int main()
{
    char server_ip[] = "172.17.0.2";
    char client_ip[] = "172.17.0.3";
    int port = 8081;
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    
    printf("[+]Server socket created successfully.\n");
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // pin to a local address and port
    struct sockaddr_in src = {0};
    src.sin_family = AF_INET;
    inet_pton(AF_INET, client_ip, &src.sin_addr);
    src.sin_port = htons(8081); // or 0 for ephemeral but fixed source IP

    if (bind(sockfd, (struct sockaddr*)&src, sizeof(src)) < 0)
    {
        perror("Error in bind");
    }
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[-]Error in connect");
        exit(1);
    }

    printf("[+]Connected to Server.\n");

    //receive_file(sockfd);
    send_file(sockfd, "file1.txt");
    printf("[+]File data sent successfully.\n");
    
    printf("[+]Closing the connection.\n");
    close(sockfd);
    
    return 0;
}
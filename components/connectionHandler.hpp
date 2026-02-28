#pragma once 

#include <string>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <functional>

class ConnectionHandler
{
private:
    std::vector<int> connected_sockets;
    std::thread accepting_thread;
    int sockfd;
    int e;
    int port;
    struct sockaddr_in server_addr;
    std::string master_ip;
    bool isServerModeOn = false, isServerRunning = false;
    std::function<void(int)> OnNewClientConnected = nullptr;
    std::function<void()> OnAllClientsDisconnected = nullptr;

    void MakeTcpConnectionKeepAlive(int sockfd)
    {
        int enable = 1;
        int isKeepAliveSuccessful = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
        if (isKeepAliveSuccessful < 0)
        {
            perror("[-]Error in setting keep-alive option");
            exit(1);
        }

        int idle = 60;      // Start probes after 60 seconds of idle
        int interval = 10;  // Probe interval: 10 seconds
        int count = 5;      // Number of probes before considering dead

        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    }

public:
    ConnectionHandler(
        int port,
        std::function<void(int)> OnNewClientConnected = nullptr,
        std::function<void()> OnAllClientsDisconnected = nullptr)
    {
        this->port = port;
        this->OnNewClientConnected = OnNewClientConnected;
        this->OnAllClientsDisconnected = OnAllClientsDisconnected;
    }
    
    int SetupSocket()
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("[-]Error in socket");
            exit(1);
        }

        this->MakeTcpConnectionKeepAlive(sockfd);
        printf("[+] Socket created successfully.\n");
        
        server_addr.sin_family = AF_INET;		
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        
        e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (e < 0)
        {
            perror("[-]Error in bind");
            exit(1);
        }

        printf("[+]Binding successfull.\n");

        return sockfd;
    }

    void StartServer()
    {
        isServerModeOn = true;
        printf("[-]Running in server mode.\n");
        if (listen(sockfd, 100) == 0)
        {
            printf("[+]Listening....\n");
        }
        else
        {
            perror("[-]Error in listening");
            exit(1);
        }

        accepting_thread = std::thread(&ConnectionHandler::StartAcceptingClients, this);
    }

    void StartAcceptingClients()
    {
        this->isServerRunning = true;
        while(this->isServerRunning)
        {
            struct sockaddr_in new_addr;
            socklen_t addr_size = sizeof(new_addr);
            int new_sock = accept(sockfd, (struct sockaddr*) &new_addr, &addr_size);
            if (new_sock < 0)
            {
                if (!isServerRunning) break;  // Socket was closed intentionally
                perror("[-]Error in accepting connection");
                continue;
            }
            
            printf("[+]Accepted connection from %s:%d\n", inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));
            connected_sockets.push_back(new_sock);
            if (OnNewClientConnected)
            {
                OnNewClientConnected(new_sock);
            }
        }

        printf("[+]Stopped accepting new clients.\n");
    }
    
    void MakeConnectionWithMaster(std::string master_ip)
    {
        if (this->isServerModeOn || this->isServerRunning)
        {
            printf("[-] Host is the current master. Cannot start a client mode.\n");
            return;
        }

        printf("[-]Running in client mode.\n");
        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(this->port);
        client_addr.sin_addr.s_addr = inet_addr(master_ip.c_str());
        connect(this->sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr));
    }

    void StopServer()
    {
        printf("[+]Stopping server and closing connections.\n");
    
        this->isServerRunning = false;
        shutdown(this->sockfd, SHUT_RDWR); // Unblocks accept()
        close(this->sockfd); // Close before join

        // Close all accepted client connections
        for (int sock : connected_sockets)
        {
            close(sock);
        }

        if (this->OnAllClientsDisconnected)
        {
            OnAllClientsDisconnected();
        }

        connected_sockets.clear();
        if (accepting_thread.joinable())
        {
            accepting_thread.join();
        }
    }
};

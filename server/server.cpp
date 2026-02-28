#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <thread>
#include <netinet/tcp.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include "components/fileSocketOperations.hpp"
#include "components/fileManager.hpp"
#include "components/fileWatcher.hpp"

using namespace std;

void process_cmd_line_args(int argc, char* argv[], string &ip, bool &isMaster, bool &hasLatestFile);
vector<int> scan_network_and_connect(int port, int timeout_ms = 100);
int main(int argc, char* argv[])
{
	string ip;
	bool isMaster, hasLatestFile;
	FileSocketOperations* fileOps = new FileSocketOperations();

	process_cmd_line_args(argc, argv, ip, isMaster, hasLatestFile);
	ConnectionHandler handler = ConnectionHandler(
		(int)8081,
		[fileOps](int sockfd) { fileOps->OnNewClientConnected(sockfd); },
		[fileOps]() { fileOps->OnAllClientsDisconnected(); });

	int sockfd = handler.SetupSocket();
	FileManager* fileManager = new FileManager();
	FileWatcher* fileWatcher = new FileWatcher(fileManager);
	fileWatcher->WatchAllFiles();
	if (isMaster)
	{
		handler.StartServer();
		int tmp;
		cout << "Press any key to stop the server and close connections..." << endl;
		cin >> tmp;
	}
	else
	{
		handler.MakeConnectionWithMaster(ip);
		if (hasLatestFile)
		{
			printf("[+]Client has the latest file. Sending to server.\n");
			fileOps->SendFile(sockfd, "/app/filesToShare/file1.txt");
		}
	}
	
	handler.StopServer();
	// fileOps.StopReceivingFiles();
	
	return 0;
}

void process_cmd_line_args(int argc, char* argv[], string &ip, bool &isMaster, bool &hasLatestFile)
{
	if (argc > 4)
	{
		perror("Not accpepting more than 1 argument.");
		exit(1);
	}

	isMaster = true;
	hasLatestFile = false;
	if (argc == 1)
	{
		printf("[+]Running in server mode.\n");
		isMaster = true;
	}
	else if (argc == 2)
	{
		perror("[+] Invalid arguments");
		exit(1);
	}
	else if (argc == 3)
	{
		if ((strcmp(argv[2], "--client") != 0 && strcmp(argv[2], "-c") != 0))
		{
			perror("Invalid argument. Use '--client' or '-c' to run in client mode.");
			exit(1);
		}

		isMaster = false;
	}
	else if (argc == 4)
	{
		if ((strcmp(argv[2], "--client") != 0 && strcmp(argv[2], "-c") != 0) ||
		    (strcmp(argv[3], "--latest-file") != 0 && strcmp(argv[3], "-l") != 0))
		{
			perror("Invalid arguments. Use '--client' or '-c' to run in client mode and '--latest-file' or '-l' to indicate having the latest file.");
			exit(1);
		}

		isMaster = false;
		hasLatestFile = true;
	}

	if (argc > 1)
		ip = argv[1];
}


// // Scan all IPs in the local network and connect to those accepting on the given port
// vector<int> scan_network_and_connect(int port, int timeout_ms)
// {
// 	vector<int> connected_sockets;
// 	struct ifaddrs *ifaddr, *ifa;
	
// 	if (getifaddrs(&ifaddr) == -1)
// 	{
// 		perror("[-]Error getting network interfaces");
// 		return connected_sockets;
// 	}

// 	// Iterate through all network interfaces
// 	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
// 	{
// 		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
// 			continue;
		
// 		// Skip loopback interface
// 		if (strcmp(ifa->ifa_name, "lo") == 0)
// 			continue;

// 		struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
// 		struct sockaddr_in *mask = (struct sockaddr_in *)ifa->ifa_netmask;
		
// 		uint32_t ip = ntohl(addr->sin_addr.s_addr);
// 		uint32_t netmask = ntohl(mask->sin_addr.s_addr);
// 		uint32_t network = ip & netmask;
// 		uint32_t broadcast = network | ~netmask;
		
// 		printf("[+]Scanning network on interface %s: %s/%d\n", 
// 			ifa->ifa_name, inet_ntoa(addr->sin_addr), __builtin_popcount(netmask));

// 		// Iterate through all hosts in the subnet (skip network and broadcast addresses)
// 		for (uint32_t host_ip = network + 1; host_ip < broadcast; host_ip++)
// 		{
// 			// Skip our own IP
// 			if (host_ip == ip)
// 				continue;

// 			struct in_addr target_addr;
// 			target_addr.s_addr = htonl(host_ip);
			
// 			// Create socket
// 			int sock = socket(AF_INET, SOCK_STREAM, 0);
// 			if (sock < 0)
// 				continue;

// 			// Set socket to non-blocking
// 			int flags = fcntl(sock, F_GETFL, 0);
// 			fcntl(sock, F_SETFL, flags | O_NONBLOCK);

// 			struct sockaddr_in target;
// 			target.sin_family = AF_INET;
// 			target.sin_port = htons(port);
// 			target.sin_addr = target_addr;

// 			// Attempt connection (non-blocking)
// 			int result = connect(sock, (struct sockaddr*)&target, sizeof(target));
			
// 			if (result == 0)
// 			{
// 				// Immediate connection (unlikely but possible)
// 				printf("[+]Connected to %s:%d\n", inet_ntoa(target_addr), port);
// 				fcntl(sock, F_SETFL, flags); // Restore blocking mode
// 				//make_tcp_connection_keep_alive(sock);
// 				connected_sockets.push_back(sock);
// 			}
// 			else if (errno == EINPROGRESS)
// 			{
// 				// Connection in progress, use select to wait
// 				fd_set write_fds;
// 				FD_ZERO(&write_fds);
// 				FD_SET(sock, &write_fds);
				
// 				struct timeval tv;
// 				tv.tv_sec = timeout_ms / 1000;
// 				tv.tv_usec = (timeout_ms % 1000) * 1000;
				
// 				result = select(sock + 1, NULL, &write_fds, NULL, &tv);
				
// 				if (result > 0)
// 				{
// 					// Check if connection succeeded
// 					int error = 0;
// 					socklen_t len = sizeof(error);
// 					getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
					
// 					if (error == 0)
// 					{
// 						printf("[+]Connected to %s:%d\n", inet_ntoa(target_addr), port);
// 						fcntl(sock, F_SETFL, flags); // Restore blocking mode
// 						//make_tcp_connection_keep_alive(sock);
// 						connected_sockets.push_back(sock);
// 					}
// 					else
// 					{
// 						close(sock);
// 					}
// 				}
// 				else
// 				{
// 					// Timeout or error
// 					close(sock);
// 				}
// 			}
// 			else
// 			{
// 				// Connection failed immediately
// 				close(sock);
// 			}
// 		}
// 	}

// 	freeifaddrs(ifaddr);
// 	printf("[+]Network scan complete. Found %zu active connections.\n", connected_sockets.size());
// 	return connected_sockets;
// }
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "contracts/FileMetaData.hpp"
#include <string>
#include <map>

using namespace std;

void handleNewConnection(int sockfd)
{
	int n;
	FILE *fp;
	int bufferSize = sizeof(struct FileMetaData);
	struct FileMetaData fileMetaData;
	map<string, FILE*> fileMap;
	
	while (1)
	{
		n = recv(sockfd, &fileMetaData, bufferSize, 0);
		if (n <= 0)
		{
			break;
		}
		
		printf("[+]Received file name: %s\n", fileMetaData.fileName);
		printf("[+]Received file type: %s\n", fileMetaData.fileType);
		printf("[+]Received file content: %s\n", fileMetaData.content);
		
		if (fileMap.find(fileMetaData.fileName) != fileMap.end())
		{
			fp = fileMap[fileMetaData.fileName];
		}
		else
		{
			fp = fopen(fileMetaData.fileName, "w");
			if (fp == NULL)
			{
				perror("[-]Error in opening file.");
				exit(1);
			}
			fileMap[fileMetaData.fileName] = fp;
		}

		fprintf(fp, "%s", fileMetaData.content);
		bzero(&fileMetaData, bufferSize);
	}

	// Close all opened files
	for (auto const& pair : fileMap)
	{
		fclose(pair.second);
	}

	return;
}
	
int main()
{
	char ip[] = "172.17.0.2";
	int port = 8081;
	int e;
	
	int sockfd, new_sock;
	struct sockaddr_in server_addr, new_addr;
	socklen_t addr_size;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("[-]Error in socket");
		exit(1);
	}
	printf("[+]Server socket created successfully.\n");
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip);
	
	e = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (e < 0)
	{
		perror("[-]Error in bind");
		exit(1);
	}
	printf("[+]Binding successfull.\n");
	
	if (listen(sockfd, 10) == 0) //  10: how many established connections can be queued
	{
		printf("[+]Listening....\n");
	}
	else
	{
		perror("[-]Error in listening");
		exit(1);
	}
	
	addr_size = sizeof(new_addr);
	new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
	handleNewConnection(new_sock);
	printf("[+]Data written in the file successfully.\n");

	close(new_sock);
	close(sockfd);
	
	return 0;
}

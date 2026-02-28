#pragma once

#include <string>
#include <components/connectionHandler.hpp>
#include <libgen.h>
#include <map>
#include <iostream>
#include <sys/stat.h>
#include <cstring>
#include <thread>
#include <functional>
#include "fileWatcher.hpp"

#define SIZE 1024
struct FileMetaData
{
	int fileNameSize;
	char fileName[256];
	int fileTypeSize;
	char fileType[256];
	char content[SIZE];
};

class FileSocketOperations
{
	private:
	ConnectionHandler* connectionHandler = nullptr;
	std::vector<std::thread> threadsToTrackReceiveFiles;
	
	void CreateDirectories(const char* path)
	{
		char* pathCopy = strdup(path);
		char* dir = dirname(pathCopy);
		
		if (strlen(dir) > 0 && strcmp(dir, ".") != 0 && strcmp(dir, "/") != 0)
		{
			// Recursively create parent directories
			CreateDirectories(dir);
			
			struct stat st;
			if (stat(dir, &st) != 0)
			{
				mkdir(dir, 0755);
				printf("[+]Created directory: %s\n", dir);
			}
		}

		free(pathCopy);
	}

	void StartReceivingFiles(int sockfd)
	{
		int n;
		FILE *fp;
		int bufferSize = sizeof(struct FileMetaData);
		struct FileMetaData fileMetaData;
		std::map<std::string, FILE*> fileMap;
		
		while (1)
		{
			n = recv(sockfd, &fileMetaData, bufferSize, 0);
			printf("[+]Received data of size: %d\n", n);

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
				// Create directories if they don't exist
				CreateDirectories(fileMetaData.fileName);
				
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

	public:
	FileSocketOperations()
	{
		
	}

	void OnNewClientConnected(int sockfd)
	{
		threadsToTrackReceiveFiles.emplace_back(&FileSocketOperations::StartReceivingFiles, this, sockfd);
	}

	void OnAllClientsDisconnected()
	{
		for (std::thread &t : this->threadsToTrackReceiveFiles)
		{
			if (t.joinable())
			{
				t.join();
			}
		}
	}

	void SendFile(int sockfd, std::string fileName)
	{
		char fileType[] = "text/plain";
		char data[SIZE] = {0};
		FILE* fp = fopen(fileName.c_str(), "r");
		if (fp == NULL)
		{
			perror("[-]Error in reading file.");
			exit(1);
		}

		while (fgets(data, SIZE, fp) != NULL)
		{
			struct FileMetaData metaData;
			metaData.fileNameSize = fileName.size();
			strcpy(metaData.fileName, fileName.c_str());
			metaData.fileTypeSize = strlen(fileType);
			strcpy(metaData.fileType, fileType);
			strcpy(metaData.content, data);

			if (send(sockfd, &metaData, sizeof(metaData), 0) == -1)
			{
				perror("[-]Error in sending file.");
				exit(1);  
			}

			bzero(data, SIZE);
		}
	}

	void AcquireFileLocksForPeer( )
	{
		// Implementation for acquiring file locks for peer
	}
};
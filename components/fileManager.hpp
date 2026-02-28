#pragma once

#include <cstdio>
#include <string>

class FileManager
{
    public:
    FileManager()
    {
    }

    bool TryToAcquireLockForPeer(const std::string& filePath)
    {
        if (this->DoesThePeerHasLockToPeformWrite(filePath))
        {
            printf("Peer already has lock to perform write: %s\n", filePath.c_str());
            return true; // Peer already has lock, cannot acquire
        }

        // printf("Trying to acquire lock for peer: %s\n", filePath.c_str());
        return true; // Assume lock is acquired for demonstration
    }

    bool DoesThePeerHasLockToPeformWrite(const std::string& filePath)
    {
        // printf("Host has lock to perform write: %s\n", filePath.c_str());
        return true;
    }

    bool AcquireLockForPeer(const std::string& filePath)
    {
        

        // printf("Acquiring lock for peer: %s\n", filePath.c_str());
        return true; // Assume lock is acquired for demonstration
    }
};
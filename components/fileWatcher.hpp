#pragma once

#include <string>
#include <sys/fanotify.h>
#include <fcntl.h>      // For AT_FDCWD, O_RDONLY
#include <unistd.h>     // For read(), write(), close()
#include <linux/limits.h>  // For PATH_MAX (if getting file paths)
#include <functional>
#include <thread>
#include <chrono>
#include "fileManager.hpp"

class FileWatcher
{
    private:
    FileManager* fileManager = nullptr;
    std::function<void(const std::string&)> onFileModifiedCallback = nullptr;
    std::vector<std::thread> watchingThreads;

    public:
    FileWatcher(FileManager* fileManager, std::function<void(const std::string&)> onFileModifiedCallback = nullptr)
    {
        this->fileManager = fileManager;
        this->onFileModifiedCallback = onFileModifiedCallback;
    }

    void WatchAllFiles()
    {
        this->watchingThreads.emplace_back(&FileWatcher::StartWatching, this, "/app/filesToShare/file1.txt");
    }

    void StartWatching(const std::string& filePath)
    {
        int fan_fd = fanotify_init(
            FAN_CLASS_CONTENT |    // Permission events (can allow/deny)
            FAN_CLOEXEC |          // Close on exec
            FAN_NONBLOCK,          // Non-blocking reads (optional)
            O_RDONLY               // Open files read-only when checking
        );

        if (fan_fd < 0)
        {
            perror("fanotify_init failed (need root?)");
            return;
        }
        
        printf("fanotify initialized (fd=%d)\n", fan_fd);
        const char* watch_path = filePath.c_str();
            int ret = fanotify_mark(
            fan_fd,
            FAN_MARK_ADD,          // Add this mark.
            // Events to monitor:
            FAN_OPEN_PERM |        // Permission check before open
            FAN_ACCESS_PERM |      // Permission check before read
            FAN_CLOSE_WRITE,       // Notification after write+close
            AT_FDCWD,              // Relative to current directory
            watch_path             // Path to watch
        );
        
            
        if (ret < 0) {
            perror("fanotify_mark failed");
            close(fan_fd);
            return;
        }
        
        printf("Watching: %s\n", watch_path);
        printf("Waiting for events... (Ctrl+C to stop)\n\n");

        char buffer[8192];

        while (1)
        {
            // Read events (blocks if FAN_NONBLOCK not set)
            ssize_t len = read(fan_fd, buffer, sizeof(buffer));
            
            if (len <= 0) {
                if (len < 0 && errno != EAGAIN)
                    perror("read");
                continue;
            }
            
            // Process each event
            struct fanotify_event_metadata* event = 
                (struct fanotify_event_metadata*)buffer;
            
            while (FAN_EVENT_OK(event, len))
            {
                bool peerHasLock = this->fileManager->TryToAcquireLockForPeer(filePath);
                struct fanotify_response response;
                response.fd = event->fd;
                response.response = peerHasLock ? FAN_ALLOW : FAN_DENY;
                
                if (write(fan_fd, &response, sizeof(response)) < 0)
                {
                    perror("write");
                }

                close(event->fd);
                event = FAN_EVENT_NEXT(event, len);

                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
};
#define SIZE 1024

struct FileMetaData
{
        int fileNameSize;
        char fileName[256];
        int fileTypeSize;
        char fileType[256];
        char content[SIZE];
};
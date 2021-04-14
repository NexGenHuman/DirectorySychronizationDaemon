#include "functions.h"
#include "sys/stat.h"
#include "time.h"
#include "utime.h"
#include "sys/mman.h"
#include "fcntl.h"
#define SMALL_FILE 1024*1024*64

void Handler(char *nazwa)
{
    syslog(LOG_INFO, "Waking up daemon through SIGUSR1");
}

//TODO ERROR HANDLING
//Return value true means that modification time of file in path1 is after modification time of path2
bool CheckIfChanged(char *path1, char *path2)
{
    struct stat filestat1, filestat2;

    stat(path1, &filestat1);
    stat(path2, &filestat2);
    
    //Negative difftime means that time1 is before time2
    if(difftime(ctime(&filestat1.st_mtime), ctime(&filestat2.st_mtime)) < 0)
        return false;
    return true;
}

//TODO ERROR HANDLING
//path2 is updated to path1 state
void UpdateFile(char *path1, char *path2)
{
    struct stat filestat;

    stat(path1, &filestat);
    if(&filestat.st_size <= SMALL_FILE)
        SwapSmall(path1, path2);
    SwapBig(path1, path2);
}

void SwapSmall(char *path1, char *path2)
{

}

void SwapBig(char *path1, char *path2)
{
    struct stat filestat;
    char* map1;
    char* map2;

    stat(path1, &filestat);

    __off64_t size = &filestat.st_size;

    int fd1 = open(path1, O_RDONLY);
    int fd2 = open(path2, O_WRONLY);

    map1 = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd1, 0);

    truncate(path2, size);
    map2 = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd2, 0);

    for(int i = 0; i < size; i++)
        map2[i]=map2[i];

    munmap(fd1, size);
    munmap(fd2, size);
    
    close(fd1);
    close(fd2);
}
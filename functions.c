#include "functions.h"
#include "sys/stat.h"
#include "time.h"
#include "utime.h"
#define SMALL_FILE 1024*1024*4

void Handler(char *nazwa)
{
    syslog(LOG_INFO, "Waking up daemon through SIGUSR1");
}

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

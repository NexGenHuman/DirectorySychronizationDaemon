#include "functions.h"
#include "sys/stat.h"
#include "time.h"
#include "utime.h"
#define SMALL_FILE 1024*1024*64

void Handler(char *nazwa)
{
    syslog(LOG_INFO, "Waking up daemon through SIGUSR1");
}

//TODO ERROR HANDLING <------ zrobione
//Return value true means that modification time of file in path1 is after modification time of path2
bool CheckIfChanged(char *path1, char *path2)
{
    struct stat filestat1, filestat2;

    if(stat(path1, &filestat1) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (CheckIfChanged)", path1);
    }
    if(stat(path2, &filestat2) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (CheckIfChanged)", path2);
    }
    
    //Negative difftime means that time1 is before time2
    if(difftime((time_t)&filestat1.st_mtime, (time_t)&filestat2.st_mtime) < 0) //nie jestem pewien czy (time_t) dziala
        return false;
    return true;
}

//TODO ERROR HANDLING <------ zrobione
//path2 is updated to path1 state
void UpdateFile(char *path1, char *path2)
{
    struct stat filestat;

    if(stat(path1, &filestat) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (UpdateFile)", path1);
    }
    if((off_t)&filestat.st_size <= SMALL_FILE) //to tez nwm czy dziala
        SwapSmall(path1, path2);
    SwapBig(path1, path2);
}

void SwapSmall(char *path1, char *path2)
{
    char buf[64];
    int file1, file2;
    int bufsize1, bufsize2;
    file1 = open(path1, O_RDONLY, 0644);
    file2 = open(path2, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(file1 == -1 || file2 == -1)
    {
        syslog(LOG_ERR, "Error opening file (SwapSmall)");
        exit(EXIT_FAILURE);
    }
    while(bufsize1 = read(file1, buf, sizeof(buf)) > 0)
    {
        bufsize2 = write(file2, buf, bufsize1);
        if(bufsize1 != bufsize2)
        {
            syslog(LOG_ERR, "Error copying file (SwapSmall)");
            exit(EXIT_FAILURE);
        }
        bzero(buf, sizeof(buf));
    }
    close(file1);
    close(file2);
    //mozliwe ze trzeba jeszcze czas zamienic
    syslog(LOG_INFO, "Copied file: %s (SwapSmall)", path1);
}

void SwapBig(char *path1, char *path2)
{
    //Trochę już o mmap poczytałem i postaram się ja tą funkcję zrobić
}
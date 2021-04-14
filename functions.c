#include "functions.h"
#include "sys/stat.h"
#include "time.h"
#include "utime.h"
#include "sys/mman.h"
#include "fcntl.h"
#define SMALL_FILE 1024 * 1024 * 64

void Handler(char *nazwa)
{
    syslog(LOG_INFO, "Waking up daemon through SIGUSR1");
}

//TODO ERROR HANDLING <------ zrobione
//Return value true means that modification time of file in path1 is after modification time of path2
bool CheckIfChanged(char *path1, char *path2)
{
    struct stat filestat1, filestat2;
    syslog(LOG_INFO, "CIC");
    if (stat(path1, &filestat1) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (CheckIfChanged)", path1);
    }
    if (stat(path2, &filestat2) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (CheckIfChanged)", path2);
    }

    //Negative difftime means that time1 is before time2
    if (difftime((time_t)&filestat1.st_mtime, (time_t)&filestat2.st_mtime) < 0) //nie jestem pewien czy (time_t) dziala
        return false;
    return true;
}

//TODO ERROR HANDLING <------ zrobione
//path2 is updated to path1 state
void UpdateFile(char *path1, char *path2)
{
    struct stat filestat;

    if (stat(path1, &filestat) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (UpdateFile)", path1);
    }
    if ((off_t)&filestat.st_size <= SMALL_FILE) //to tez nwm czy dziala
        SwapSmall(path1, path2);
    SwapBig(path1, path2);
}

void Compare(char *path1, char *path2, bool recursion, int filesize) //porownuje foldery i wywoluje usuwanie/kopiowanie
{
    char entry_path1[PATH_MAX + 1], entry_path2[PATH_MAX + 1];
    size_t path_len1, path_len2;
    DIR *dir1, *dir2;
    struct dirent *file1, *file2;
    bool same;
    strncpy(entry_path1, path1, sizeof(entry_path1));
    path_len1 = strlen(path1);
    strncpy(entry_path2, path2, sizeof(entry_path2));
    path_len2 = strlen(path2);
    if (entry_path1[path_len1 - 1] != '/')
    {
        entry_path1[path_len1] = '/';
        entry_path1[path_len1 + 1] = '\0';
        ++path_len1;
    }
    if (entry_path2[path_len2 - 1] != '/')
    {
        entry_path2[path_len2] = '/';
        entry_path2[path_len2 + 1] = '\0';
        ++path_len2;
    }
    dir2 = opendir(path2);
    while ((file2 = readdir(dir2)) != NULL)
    {
        same = false;
        dir1 = opendir(path1);
        strncpy(entry_path2 + path_len2, file2->d_name, sizeof(entry_path2) - path_len2);
        struct stat st1;
        lstat(entry_path1, &st1);
        if (S_ISDIR(st1.st_mode))
        {
            //jezeli folder
        }
        else
        {
            while ((file1 = readdir(dir1)) != NULL)
            {
                strncpy(entry_path1 + path_len1, file1->d_name, sizeof(entry_path1) - path_len1);
                if (file1->d_name == file2->d_name)
                {
                    same = true;
                    break;
                }
            }
            if (same == false)
            {
                Delete(entry_path2);
            }
            closedir(dir1);
        }
        //syslog(LOG_INFO, "%s", entry_path2);
    }
    closedir(dir2);
    dir1 = opendir(path1);
    while ((file1 = readdir(dir1)) != NULL)
    {
        same = false;
        dir2 = opendir(path2);
        strncpy(entry_path1 + path_len1, file1->d_name, sizeof(entry_path1) - path_len1);
        struct stat st1;
        lstat(entry_path1, &st1);
        if (S_ISDIR(st1.st_mode))
        {
            //jezeli folder
        }
        else
        {
            //syslog(LOG_INFO, "%s", entry_path2);
            while ((file2 = readdir(dir2)) != NULL)
            {
                strncpy(entry_path2 + path_len2, file2->d_name, sizeof(entry_path2) - path_len2);
                if (file1->d_name == file2->d_name)
                {
                    if (CheckIfChanged(entry_path1, entry_path2))
                    {
                        UpdateFile(entry_path1, entry_path2);
                    }
                    same = true;
                    break;
                }
            }
            if (same == false)
            {
                UpdateFile(entry_path1, strncpy(entry_path2 + path_len2, file1->d_name, sizeof(entry_path2) - path_len2));
            }
            closedir(dir2);
        }
    }
    closedir(dir1);
}

void SwapSmall(char *path1, char *path2)
{
    char buf[64];
    int file1, file2;
    int bufsize1, bufsize2;
    file1 = open(path1, O_RDONLY, 0644);
    file2 = open(path2, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (file1 == -1 || file2 == -1)
    {
        syslog(LOG_ERR, "Error opening file (SwapSmall)");
        exit(EXIT_FAILURE);
    }
    while (bufsize1 = read(file1, buf, sizeof(buf)) > 0)
    {
        bufsize2 = write(file2, buf, bufsize1);
        if (bufsize1 != bufsize2)
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
    struct stat filestat;
    char *map1;
    char *map2;

    if (stat(path1, &filestat) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (SwapBig)", path1);
        exit(EXIT_FAILURE);
    }

    __off64_t size = &filestat.st_size;

    int fd1 = open(path1, O_RDONLY);
    if (fd1 == -1)
    {
        syslog(LOG_ERR, "Error opening file (SwapBig)");
        exit(EXIT_FAILURE);
    }
    int fd2 = open(path2, O_WRONLY);
    if (fd2 == -1)
    {
        syslog(LOG_ERR, "Error opening file (SwapBig)");
        exit(EXIT_FAILURE);
    }

    map1 = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd1, 0);
    if (map1 == MAP_FAILED)
    {
        syslog(LOG_ERR, "Error mapping file: %s (SwapBig)", path1);
        exit(EXIT_FAILURE);
    }

    if (truncate(path2, size) == -1)
    {
        syslog(LOG_ERR, "Error truncating file: %s (SwapBig)", path2);
        exit(EXIT_FAILURE);
    }
    map2 = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd2, 0);
    if (map1 == MAP_FAILED)
    {
        syslog(LOG_ERR, "Error mapping file: %s (SwapBig)", path2);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; i++)
        map2[i] = map2[i];

    munmap(fd1, size);
    munmap(fd2, size);

    close(fd1);
    close(fd2);

    syslog(LOG_INFO, "Copied file: %s (SwapBig)", path1);
}

void Delete(char *path)
{
    struct stat filestat;

    stat(path, &filestat);
    if (S_ISREG(filestat.st_mode))
    {
        // Handle regular file
        if (unlink(path) == -1)
        {
            syslog(LOG_ERR, "Error deleting file: %s (Delete)", path);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Handle directories
        DIR *d;
        struct dirent *dir;
        d = opendir(path);
        while ((dir = readdir(d)) != NULL)
        {
            char *extendedPath = (char *)malloc(1 + strlen(path) + strlen(dir->d_name));
            strcpy(extendedPath, path);
            strcat(extendedPath, dir->d_name);
            Delete(extendedPath);
        }
        closedir(d);

        rmdir(path);
    }
}

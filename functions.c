#include "functions.h"
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <sys/mman.h>
#include <fcntl.h>
#define SMALL_FILE 1024 * 1024 * 64

void Handler(int signum)
{
    syslog(LOG_INFO, "Waking up daemon through SIGUSR1");
}

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
    if (difftime((time_t)filestat1.st_mtime, (time_t)filestat2.st_mtime) < 0) //nie jestem pewien czy (time_t) dziala
        return false;
    else
        return true;
}

//path2 is updated to path1 state
void UpdateFile(char *path1, char *path2)
{
    struct stat filestat;

    if (stat(path1, &filestat) == -1)
    {
        syslog(LOG_ERR, "Error retriveing information about the file: %s (UpdateFile)", path1);
    }
    if ((off_t)filestat.st_size <= SMALL_FILE) //to tez nwm czy dziala
        SwapSmall(path1, path2);
    else
    {
        SwapBig(path1, path2);
    }
}

void Compare(char *path1, char *path2, bool recursion, int filesize) //porownuje foldery i wywoluje usuwanie/kopiowanie
{
    //syslog(LOG_INFO, "Compare: %s, %s", path1, path2);
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
        strncpy(entry_path2 + path_len2, file2->d_name, sizeof(entry_path2) - path_len2);
        //syslog(LOG_INFO, "Compare (del1) - reading file: %s", file2->d_name);
        if (!((strcmp(file2->d_name, ".") == 0 || strcmp(file2->d_name, "..") == 0)))
        {
            //syslog(LOG_INFO, "Compare (del2) - reading file: %s", entry_path2);
            struct stat st1;
            lstat(entry_path2, &st1);
            if (S_ISDIR(st1.st_mode) && recursion)
            {
                dir1 = opendir(path1);
                while ((file1 = readdir(dir1)) != NULL)
                {
                    strncpy(entry_path1 + path_len1, file1->d_name, sizeof(entry_path1) - path_len1);
                    char entry_path_cp[PATH_MAX + 1];
                    strncpy(entry_path_cp, entry_path2, sizeof(entry_path_cp));
                    strncpy(entry_path_cp + path_len2, file1->d_name, sizeof(entry_path2) - path_len2);
                    //syslog(LOG_INFO, "Compare (deldir): %s, %s, %s,", entry_path1, entry_path2, entry_path_cp);
                    if (strcmp(entry_path_cp, entry_path2) == 0)
                    {
                        Compare(entry_path1, entry_path2, recursion, filesize);
                        same = true;
                        break;
                    }
                }
                if (same == false)
                {
                    //syslog(LOG_INFO, "Compare (deldir) - reading file: %s", entry_path2);
                    Delete(entry_path2);
                }
                closedir(dir1);
            }
            else if (!S_ISDIR(st1.st_mode))
            {
                //syslog(LOG_INFO, "Compare (delfile): %s", path1);
                dir1 = opendir(path1);
                while ((file1 = readdir(dir1)) != NULL)
                {
                    strncpy(entry_path1 + path_len1, file1->d_name, sizeof(entry_path1) - path_len1);
                    //syslog(LOG_INFO, "Compare (delfile) - reading file2: %s", entry_path1);
                    if (file1->d_name == file2->d_name)
                    {
                        same = true;
                        break;
                    }
                }
                if (same == false)
                {
                    //syslog(LOG_INFO, "Compare (delfile) - del file: %s", entry_path2);
                    Delete(entry_path2);
                }
                closedir(dir1);
            }
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
        //syslog(LOG_INFO, "Compare (cp1) - reading file: %s", file1->d_name);
        if (!((strcmp(file1->d_name, ".") == 0 || strcmp(file1->d_name, "..") == 0)))
        {
            //syslog(LOG_INFO, "Compare (cp2) - reading file: %s", entry_path1);
            if (S_ISDIR(st1.st_mode) && recursion)
            {
                while ((file2 = readdir(dir2)) != NULL)
                {
                    strncpy(entry_path2 + path_len2, file2->d_name, sizeof(entry_path2) - path_len2);
                    if (file1->d_name == file2->d_name)
                    {
                        //char *newdirpath = strncpy(entry_path2 + path_len2, "/", sizeof(entry_path2) - path_len2);
                        //mkdir(path2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        Compare(entry_path1, entry_path2, recursion, filesize);
                        same = true;
                        break;
                    }
                }
                if (same == false)
                {
                    strncpy(entry_path2 + path_len2, file1->d_name, sizeof(entry_path2) - path_len2);
                    mkdir(entry_path2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    Compare(entry_path1, entry_path2, recursion, filesize);
                    //UpdateFile(entry_path1, strncpy(entry_path2 + path_len2, file1->d_name, sizeof(entry_path2) - path_len2));
                }
                closedir(dir2);
            }
            else if (!S_ISDIR(st1.st_mode))
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
                    strncpy(entry_path2 + path_len2, file1->d_name, sizeof(entry_path2) - path_len2);
                    UpdateFile(entry_path1, entry_path2);
                }
                closedir(dir2);
            }
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
    if (file1 == -1)
    {
        syslog(LOG_ERR, "Error opening file p1: %s (SwapSmall)", path1);
        exit(EXIT_FAILURE);
    }
    if (file2 == -1)
    {
        syslog(LOG_ERR, "Error opening file p2: %s (SwapSmall)", path2);
        exit(EXIT_FAILURE);
    }
    while ((bufsize1 = read(file1, buf, sizeof(buf))) > 0)
    {
        bufsize2 = write(file2, buf, (ssize_t)bufsize1);
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

    int fd1 = open(path1, O_RDONLY);
    if (fd1 == -1)
    {
        syslog(LOG_ERR, "Error opening file (SwapBig)");
        exit(EXIT_FAILURE);
    }
    int fd2 = open(path2, O_RDWR);
    if (fd2 == -1)
    {
        syslog(LOG_ERR, "Error opening file (SwapBig)");
        exit(EXIT_FAILURE);
    }

    map1 = (char*)mmap(NULL, filestat.st_size, PROT_READ, MAP_PRIVATE, fd1, 0);
    if (map1 == MAP_FAILED)
    {
        syslog(LOG_ERR, "Error mapping file: %s (SwapBig)", path1);
        exit(EXIT_FAILURE);
    }

    if (truncate(path2, filestat.st_size) == -1)
    {
        syslog(LOG_ERR, "Error truncating file: %s (SwapBig)", path2);
        exit(EXIT_FAILURE);
    }
    map2 = (char*)mmap(NULL, filestat.st_size, PROT_WRITE, MAP_SHARED, fd2, 0);
    if (map2 == MAP_FAILED)
    {
        syslog(LOG_ERR, "Error mapping file: %s (SwapBig)", path2);
        exit(EXIT_FAILURE);
    }

    //Core dump here
    for (int i = 0; i < filestat.st_size; i++)
        map2[i] = map2[i];

    munmap(map1, filestat.st_size);
    munmap(map2, filestat.st_size);

    close(fd1);
    close(fd2);

    syslog(LOG_INFO, "Copied file: %s (SwapBig)", path1);
}

//TODO FINISH ERROR HANDLING
void Delete(char *path)
{
    struct stat filestat;
    //syslog(LOG_INFO, "Deleteing file: %s", path);
    stat(path, &filestat);
    //syslog(LOG_INFO, "Deleteing file1: %s", path);
    if (S_ISREG(filestat.st_mode))
    {
        // Handle regular file
        if (unlink(path) == -1)
        {
            syslog(LOG_ERR, "Error deleting file: %s (Delete)", path);
            exit(EXIT_FAILURE);
        }
        else
        {
            syslog(LOG_INFO, "Deleted file: %s", path);
        }
    }
    else
    {
        // Handle directories
        DIR *d;
        struct dirent *dir;
        d = opendir(path);
        //syslog(LOG_INFO, "Deleteing file23: %s", path);
        while ((dir = readdir(d)) != NULL)
        {
            if (!((strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)))
            {
                //syslog(LOG_INFO, "Deleteing file33: %s", path);
                /*char *extendedPath = (char *)malloc(1 + strlen(path) + strlen(dir->d_name));
                strcpy(extendedPath, path);
                strcat(extendedPath, dir->d_name);*/
                int path_len = strlen(path);
                char path_cp[PATH_MAX + 1];
                strncpy(path_cp, path, sizeof(path_cp));
                //syslog(LOG_INFO, "Deleteing filebbb: %s", path_cp);
                if (path_cp[path_len - 1] != '/')
                {
                    path_cp[path_len] = '/';
                    path_cp[path_len + 1] = '\0';
                    ++path_len;
                }
                //syslog(LOG_INFO, "Deleteing fileccc: %s", path_cp);
                strncpy(path_cp + path_len, dir->d_name, sizeof(path_cp) - path_len); 
                //syslog(LOG_INFO, "Deleteing filebbsbddd: %s", dir->d_name);
                Delete(path_cp);
            }
        }
        closedir(d);
        rmdir(path);
    }
}

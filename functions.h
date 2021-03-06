#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

void UpdateFile(char *path1, char *path2, int filesize);
bool CheckIfChanged(char *path1, char *path2);
void SwapSmall(char *path1, char *path2);
void SwapBig(char *path1, char *path2);
void Handler(int signum);
void Compare(char *path1, char *path2, bool recursion, int filesize);
void Delete(char *path);
#endif
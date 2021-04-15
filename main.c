#include "functions.h"

int main(int argc, char **argv)
{
    openlog("Copying_daemon", LOG_CONS, LOG_USER);

    pid_t pid, sid;

    pid = fork();
    if (pid < 0)
    {
        syslog(LOG_ERR, "Wrong pid while forking");
        write(1, "Wrong pid while forking\n", 24);
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    umask(0);

    sid = setsid();
    if (sid < 0)
    {
        syslog(LOG_ERR, "Wrong sid");
        exit(EXIT_FAILURE);
    }
    if ((chdir("/")) < 0)
    {
        syslog(LOG_ERR, "Error while changing daemon directory");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fsize;
    int s;
    int sleeptime = 300;
    bool recursion = false;
    char *in, *out;
    while ((s = getopt(argc, argv, "i:o:Rm:s:")) != -1)
        switch (s)
        {
        case 'i': //1 folder
            in = optarg;
            //syslog(LOG_INFO, "Option -R");
            break;
        case 'o': //2 folder
            out = optarg;
            //syslog(LOG_INFO, "Option -R");
            break;
        case 'R': //opcja "-R"
            recursion = true;
            //syslog(LOG_INFO, "Option -R");
            break;

        case 's': //opcja "-s sleeptime" - zmienia czas spania
            sleeptime = atoi(optarg);
            //syslog(LOG_INFO, "Option -s");
            break;

        case 'm':
            fsize = atoi(optarg);
            break;

        case '?':
            if (optopt == 's')
                syslog(LOG_ERR, "Option -s requires argument");
            else if (optopt == 'm')
                syslog(LOG_ERR, "Option -m requires argument");
            else
                syslog(LOG_ERR, "Unknown option character");
            exit(EXIT_FAILURE);
            break;

        default:
            abort();
        }
    DIR *dir_ptr;
    if ((dir_ptr = opendir(in)) == NULL || (dir_ptr = opendir(out)) == NULL)
    {
        syslog(LOG_ERR, "Wrong directory");
        write(1, "Wrong directory\n", 16);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Daemon starting");
    if (signal(SIGUSR1, Handler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Signal error");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        Compare(in, out, recursion, fsize);
        syslog(LOG_INFO, "Daemon going to sleep");
        if (sleep(sleeptime) == 0)
        {
            syslog(LOG_INFO, "Daemon waking up");
        }
    }

    closelog();
    return 0;
}

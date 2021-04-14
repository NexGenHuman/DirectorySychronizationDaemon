#include "functions.h"

int main(int argc, char **argv)
{
    openlog("Copying_daemon", LOG_CONS, LOG_USER);

    DIR *dir_ptr;
    if ((dir_ptr = opendir(argv[1])) == NULL || (dir_ptr = opendir(argv[2])) == NULL)
    {
        syslog(LOG_ERR, "Wrong directory");
        write(1, "Wrong directory\n", 16);
        exit(EXIT_FAILURE);
    }

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

    int s;
    int sleeptime = 300;
    bool recursion = false;
    while ((s = getopt(argc, argv, "Rms:")) != -1)
        switch (s)
        {
        case 'R':   //opcja "-R"
            recursion = true;
            //syslog(LOG_INFO, "Option -R");
        break;

        case 's':   //opcja "-s sleeptime" - zmienia czas spania
            sleeptime = atoi(optarg);
            //syslog(LOG_INFO, "Option -s");
            break;

        case 'm':
            //miejsce na 2 kropke
        break;

        case '?':
            if (optopt == 's')
                syslog(LOG_ERR, "Option -s requires argument");
            else
                syslog(LOG_ERR, "Unknown option character");
            exit(EXIT_FAILURE);
        break;

        default:
            abort();
        }
    syslog(LOG_INFO, "Daemon starting");
    if(signal(SIGUSR1, Handler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Signal error");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        //tutaj bedzie porownywac fordery itd
        if(sleep(sleeptime) == 0)
        {
            syslog(LOG_INFO, "Daemon waking up");
        }
    }

    closelog();
    return 0;
}

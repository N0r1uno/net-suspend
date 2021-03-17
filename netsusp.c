#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>

struct entry
{
    int port;
    char protocol;
};

int established(const struct entry con)
{
    int result = -1;
    char buf[64];
    sprintf(buf, "netstat -%c | grep -c ':%i'", con.protocol, con.port);
    FILE *fp = popen(buf, "r");
    if (!fp)
        return -1;
    if (fgets(buf, 64, fp))
        result = atoi(buf);
    pclose(fp);
    return result;
}

void f_log(const char *msg)
{
    char tstamp[26];
    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);
    strftime(tstamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    FILE *f = fopen("/var/log/netsusp.log", "a");
    fprintf(f, "[%s]:%s\n", tstamp, msg);
    fclose(f);
}

void help()
{
    printf("usage:\tnetsusp -d <delay> -<protocol> <port> [...]"
           "\n  delay:\ttime in minutes until suspend after inactivity"
           "\n  protocol:\tu(dp)/t(cp)\n  port:\t\t0-65535\n"
           "\neg: netsusp -d=30 -t:25565 -u:8192\n");
    exit(EXIT_FAILURE);
}

char r = 1;
void term()
{
    r = 0;
}

int main(int argc, char **argv)
{
    if (argc < 5 || !(argc % 2))
        help();

    const int l = (argc - 3) / 2;
    int opt, i = 0, delay = -1;
    struct entry e[l];
    opterr = 0;

    while ((opt = getopt(argc, argv, "d:u:t:")) != -1)
    {
        if (opt == 'd')
            delay = atoi(optarg);
        else if (opt == 't' || opt == 'u')
        {
            e[i].protocol = opt;
            e[i].port = atoi(optarg);
            if (e[i].port < 0 || e[i].port > 65535)
                help();
            i++;
        }
        else
            help();
    }

    if (delay < 0)
        help();

    for (i = 0; i < l; i++)
        if (established(e[i]) == -1)
        {
            fprintf(stderr, "An error occurred while fetching connections with netstat.\n");
            return EXIT_FAILURE;
        }

    if (signal(SIGTERM, term) == SIG_ERR || signal(SIGINT, term) == SIG_ERR)
    {
        fprintf(stderr, "An error occurred while setting up signals.\n");
        return EXIT_FAILURE;
    }

    f_log("Init.");

    int active, susp = 0;
    while (r)
    {
        for (i = active = 0; i < l; active += established(e[i++]))
            ;
        if (active <= 0)
        {
            if (susp >= delay)
            {
                susp = 0;
                f_log("Suspend.");
                system("systemctl suspend");
                sleep(10);
                f_log("Woke up.");
            }
            else
                susp++;
        }
        else
            susp = 0;
        sleep(60);
    }

    f_log("Exit.");
    return EXIT_SUCCESS;
}

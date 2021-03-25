#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

char _log = 0;

struct entry {
    int port;
    char protocol;
};

int only_digits(const char *s) {
    while (*s) {
        if (*s != '\n' && !isdigit(*s)) return 0;
        s++;
    }
    return 1;
}

int established(const struct entry con) {
    int result = -1;
    char buf[64] = {'\0'};
    sprintf(buf, "ss -%c | grep -c ':%i'", con.protocol, con.port);
    FILE *fp = popen(buf, "r");
    if (!fp)
        return -1;
     if (fgets(buf, 8, fp) && only_digits(buf))
        result = atoi(buf);
    pclose(fp);
    return result;
}

void f_log(const char *msg) {
    if (!_log) return;
    char tstamp[26];
    time_t timer = time(NULL);
    struct tm *tm_info = localtime(&timer);
    strftime(tstamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    FILE *f = fopen("/var/log/netsusp.log", "a");
    if (!f) {
        fprintf(stderr, "An error occurred while trying to write to the logfile. (root rights required!)\n");
        exit(EXIT_FAILURE);
    }
    fprintf(f, "[%s]:%s\n", tstamp, msg);
    fclose(f);
}

void help() {
    printf("usage:\tnetsusp -l -d <delay> -<protocol> <port> [...]"
           "\n  l:\t\twrite logs to /var/log/netsusp.log (optional)"
           "\n  delay:\ttime in minutes until suspend after inactivity"
           "\n  protocol:\tu(dp)/t(cp)\n  port:\t\t0-65535\n"
           "\neg: netsusp -d 30 -t 25565 -u 8192\n");
    exit(EXIT_FAILURE);
}

void term() {
    f_log("Exit.");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 5)
        help();

    const int l = (argc - (2 + !(argc % 2))) / 2;
    int opt, i = 0, delay = -1;
    struct entry e[l];
    opterr = 0;

    while ((opt = getopt(argc, argv, "d:u:t:l")) != -1) {
        if (opt == 'd') {
            if (!only_digits(optarg))
                help();
            delay = atoi(optarg);
        } else if (opt == 'l')
            _log = 1;
        else if (opt == 't' || opt == 'u') {
            e[i].protocol = opt;
            if (!only_digits(optarg))
                help();
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
        if (established(e[i]) == -1) {
            fprintf(stderr, "An error occurred while fetching connections with socket statistics(ss).\n");
            return EXIT_FAILURE;
        }

    if (signal(SIGTERM, term) == SIG_ERR || signal(SIGINT, term) == SIG_ERR) {
        fprintf(stderr, "An error occurred while setting up signals.\n");
        return EXIT_FAILURE;
    }

    f_log("Init.");

    printf("<--summary-->\ndelay=%imin%s\n", delay, (_log)?"\nlog enabled":"");
    for (int i = 0; i < l; i++)
        printf("%i.  %c:%i\n", i+1, e[i].protocol, e[i].port);

    int active, susp = 0;
    while (1) {
        for (i = active = 0; i < l; active += established(e[i++]));
        if (active <= 0) {
            if (susp >= delay) {
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

    return EXIT_SUCCESS;
}

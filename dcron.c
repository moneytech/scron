#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>

#define LEN 80

int main(int argc, char *argv[]) {
	char *ep, *col, line[LEN+1], config[LEN+1] = "/etc/dcron.conf";
	int i, j;
	time_t t;
	struct tm *tm;
	FILE *fp;

	openlog(argv[0], LOG_CONS | LOG_PID, LOG_LOCAL1);

	for (i = 1; i < argc; i++) {
		if (!strcmp("-d", argv[i])) {
			if (fork() == 0) {
				setsid();
				fclose(stdin);
				fclose(stdout);
				fclose(stderr);
			} else {
				return 0;
			}
		} else if (!strcmp("-f", argv[i])) {
			if (argv[i+1] == NULL || argv[i+1][0] == '-') {
				fprintf(stderr, "error: -f needs parameter\n");
				syslog(LOG_NOTICE, "error: -f needs parameter");
				return 1;
			}
			strncpy(config, argv[++i], LEN);
		} else {
			fprintf(stderr, "usage: %s [options]\n", argv[0]);
			fprintf(stderr, "-h        help\n");
			fprintf(stderr, "-d        daemon\n");
			fprintf(stderr, "-f <file> config file\n");
			return 1;
		}
	}

	while (1) {
		t = time(NULL);
		sleep(60 - t % 60);
		t = time(NULL);
		tm = localtime(&t);

		if ((fp = fopen(config, "r")) == NULL) {
			fprintf(stderr, "error: cant read %s\n", config);
			syslog(LOG_NOTICE, "error: cant read %s", config);
			continue;
		}

		for (i = 0; fgets(line, LEN+1, fp); i++) {
			if (line[0] == '#' || line[0] == '\n')
				continue;

			for (j = 0, col = strtok(line,"\t"); col; j++, col = strtok(NULL, "\t")) {
				ep = "";

				if ((j == 0 && (col[0] == '*' || strtol(col, &ep, 0) == tm->tm_min) && *ep == '\0') ||
					(j == 1 && (col[0] == '*' || strtol(col, &ep, 0) == tm->tm_hour) && *ep == '\0') ||
					(j == 2 && (col[0] == '*' || strtol(col, &ep, 0) == tm->tm_mday) && *ep == '\0') ||
					(j == 3 && (col[0] == '*' || strtol(col, &ep, 0) == tm->tm_mon) && *ep == '\0') ||
					(j == 4 && (col[0] == '*' || strtol(col, &ep, 0) == tm->tm_wday) && *ep == '\0')) {
					continue;
				} else if (j == 5) {
					printf("run: %s", col);
					syslog(LOG_NOTICE, "run: %s", col);
					if (fork() == 0) {
						execl("/bin/sh", "/bin/sh", "-c", col, (char *) NULL);
						fprintf(stderr, "error: job failed: %s", col);
						syslog(LOG_NOTICE, "error: job failed: %s", col);
					}
				} else if (!isdigit(col[0]) || *col < 0 || *col > 59 || *ep != '\0') {
					fprintf(stderr, "error: %s line %d column %d\n", config, i+1, j+1);
					syslog(LOG_NOTICE, "error: %s line %d column %d", config, i+1, j+1);
				}

				break;
			}
		}

		fclose(fp);
	}

	closelog();
	return 0;
}

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "params.h"
#include "utils.h"

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in addr;
	char sentence[8192];
	int len;
	int p;

	defaultPort = htons(21);
	strcpy(host, "127.0.0.1");

	for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-port") == 0) {
			i++;
			defaultPort = atoi(argv[i]);
		} else if (strcmp(argv[i], "-host") == 0) {
			i++;
			sprintf(host, "%s", argv[i]);
        } else {
            printf("Invalid arguments %s\n", argv[i]);
            return 1;
        }
    }

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(defaultPort);
	if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	status = STATUS_START;
	mode = THREAD_MODE_NON;
    filefd = -1;
    port = -1;

	while(1) {
		if(status == STATUS_CMD) {
			printf("->");
			fgets(sentence, 4096, stdin);
			len = strlen(sentence);
			sentence[len - 1] = '\r';
            sentence[len] = '\n';
            sentence[len + 1] = '\0';

			p = 0;
            cmd = cmdType(sentence);
			if (cmd == WRONG_CMD || cmd == ERROR) continue;
			while (p < len) {
				int n = write(sockfd, sentence + p, len + 1 - p);
				if (n < 0) {
					printf("Error write(): %s(%d)\n", strerror(errno), errno);
					return 1;
				} else {
					p += n;
				}
			}
		}

		p = 0;
		while (1) {
			int n = read(sockfd, sentence + p, 8191 - p);
			if (n < 0) {
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				return 1;
			} else if (n == 0) {
				break;
			} else {
				p += n;
				if (sentence[p - 1] == '\n') {
					break;
				}
			}
		}

        if (p)
            sentence[p] = '\0';
        else
            continue;

		if (status == STATUS_START) {
			if (strstr(sentence, "220") == sentence) {  //server: connecting success
                status = STATUS_CMD;
                printf("FROM SERVER: %s", sentence);
            }
		} else if (status == STATUS_CMD) {
            if (strstr(sentence, "451") == sentence || strstr(sentence, "550") == sentence)   //server: fail to open file
                printf("FROM SERVER: %s", sentence);
            else {
                cmdHandler(sentence);
                printf("FROM SERVER: %s", sentence);
                if (strstr(sentence, "150") == sentence)
                    status = STATUS_PROCESS;
                if (cmd == QUIT)
                    break;
            }
        } else if (status == STATUS_PROCESS) {
            printf("FROM SERVER: %s", sentence);
            status = STATUS_CMD;
        }

	}

	close(sockfd);

	return 0;
}

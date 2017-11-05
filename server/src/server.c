#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "params.h"
#include "handlers.h"

void process(Client* client){
	char sentence[8192];
	int p;
	int len;

	while(1) {
		p = 0;
		while (1) {
			int n = read(client->connfd, sentence + p, 8191 - p);
			if (n < 0) {
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				close(client->connfd);
				continue;
			} else if (n == 0) {
				break;
			} else {
				p += n;
				if (sentence[p - 1] == '\n') {
					break;
				}
			}
		}

		sentence[p - 1] = '\0';
		len = p - 1;

		clientHandler(client, sentence);

		if (client->status == STATUS_QUIT) {
			free(client);
			break;
		}
	}
	close(client->connfd);
}

int main(int argc, char **argv) {
	struct sockaddr_in addr;

    defaultPort = 21;
    strcpy(defaultRoot, "/tmp");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-port") == 0) {
            i++;
            defaultPort = atoi(argv[i]);
        } else if (strcmp(argv[i], "-root") == 0) {
            i++;
            strcpy(defaultRoot, argv[i]);
        } else {
            printf("Invalid arguments %s\n", argv[i]);
            return 1;
        }
    }
    printf("Server on port %d at root %s\n", defaultPort, defaultRoot);

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(defaultPort);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	int connfd;
	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		else{
			if (write(connfd, S220, strlen(S220)) == -1)
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
		}

		pthread_t pid;
		Client* client = (Client*)malloc(sizeof(Client));
        initClient(client);
        client->connfd = connfd;
        pthread_create(&pid, NULL, (void*)process, client);
	}
	close(listenfd);
    return 0;
}
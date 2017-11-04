//
// Created by gmx15 on 2017/11/1.
//
#include <string.h>
#include <stdlib.h>

#include "params.h"

int judgeCmd(char* sentence, char* cmd) {
    while(*cmd){
        if(*sentence == '\0')
            return 0;
        if(*cmd != *sentence)
            return 0;
        cmd++;
        sentence++;
    }
    return 1;
}

char* parseFilename(char* sentence, char* cmd) {
    char* param = (char*)malloc((sizeof(char)) * strlen(sentence));
    char* space = strchr(sentence, ' ');
    if(space != NULL)
        strcpy(param, space + 1);
    else
        *param = '\0';
    for (int i = strlen(param) - 1; i >= 0; i--){
        if(strchr(" \r\n\t", param[i]) != NULL)
            param[i] = '\0';
    }
    char* filename = (char*)malloc((sizeof(char)) * strlen(param));
    for (int i = strlen(param) - 1; i >= -1; i--) {
        if (param[i] == '/' || i == -1) {
            strcpy(filename, param + i + 1);
            break;
        }
    }
    return filename;
}

int createFilesocket() {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    if ((filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    if (connect(filefd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int cmdType(char* sentence) {
    FILE *f;
    char cmd[100];
    if (judgeCmd(sentence, "USER")) return USER;
    else if (judgeCmd(sentence, "PASS")) return PASS;
    else if (judgeCmd(sentence, "SYST")) return SYST;
    else if (judgeCmd(sentence, "TYPE")) return TYPE;
    else if (judgeCmd(sentence, "PORT")) {
        int h1, h2, h3, h4, p1, p2;
        sscanf(sentence, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
        port = p1 * 256 + p2;
        sprintf(host, "%d.%d.%d.%d", h1, h2, h3, h4);
        return PORT;
    }
    else if (judgeCmd(sentence, "PASV")) return PASV;
    else if (judgeCmd(sentence, "STOR")) {
        if (mode == THREAD_MODE_PASV) {
            if (filefd != -1)
                return STOR;
            if (createFilesocket() == -1) {
                close(filefd);
                filefd = -1;
                return ERROR;
            }
        }
        strcpy(cmd, parseFilename(sentence, "STOR"));
        f = fopen(cmd, "r");
        if (f == NULL) {
            printf("no such file: %s\n", cmd);
            close(filefd);
            filefd = -1;
            return ERROR;
        }
        fclose(f);
        strcpy(filename, cmd);
        return STOR;
    }
    else if (judgeCmd(sentence, "RETR")) {
        if (mode == THREAD_MODE_PASV) {
            if (filefd != -1)
                return RETR;
            if (createFilesocket() == -1) {
                close(filefd);
                filefd = -1;
                return ERROR;
            }
        }
        strcpy(filename, parseFilename(sentence, "STOR"));
        return RETR;
    }
    else if (judgeCmd(sentence, "PWD")) return PWD;
    else if (judgeCmd(sentence, "MKD")) return MKD;
    else if (judgeCmd(sentence, "CWD")) return CWD;
    else if (judgeCmd(sentence, "RMD")) return RMD;
    else if (judgeCmd(sentence, "LIST")) {
        if (mode == THREAD_MODE_PASV) {
            if (filefd != -1)
                return LIST;
            if (createFilesocket() == -1) {
                close(filefd);
                filefd = -1;
                return ERROR;
            }
        }
        return LIST;
    }
    else if (judgeCmd(sentence, "QUIT")) return QUIT;
    else return WRONG_CMD;
}

void cmdHandler(char* sentence) {
    struct sockaddr_in addr;
    int p, len, connfd;
    FILE* f;
    char buff[4096];
    switch (cmd) {
        case PORT:
            mode = THREAD_MODE_PORT;
            if ((filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
                printf("Error socket(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(filefd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
                printf("Error bind() on %s,%d: %s(%d)\n", host, port, strerror(errno), errno);
                return;
            }
            if (listen(filefd, 10) == -1) {
                printf("Error listen(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            break;
        case PASV:
            mode = THREAD_MODE_PASV;
            int h1, h2, h3, h4, p1, p2;
            for (int i = 0; i < 100; i++)
                host[i] = '\0';
            for (int i = 5; i < 100; i++) {
                if (sentence[i] >= '0' && sentence[i] <= '9') {
                    sscanf(sentence + i, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
                    break;
                }
            }
            sprintf(host, "%d.%d.%d.%d", h1, h2, h3, h4);
            port = htons(p1 * 256 + p2);
            break;
        case STOR:
            if (mode == THREAD_MODE_PASV)
                connfd = filefd;
            else {
                struct sockaddr addr;
                socklen_t len;
                connfd = accept(filefd, &addr, &len);
                if (connfd == -1) {
                    printf("accept error\n");
                    return;
                }
            }
            f = fopen(filename, "r");
            if (f == NULL) {
                printf("open error\n");
                return;
            }
            while (1) {
                len = fread(buff, 1, 4096, f);
                if (len == 0) break;
                p = 0;
                while (p < len) {
                    int n = write(connfd, buff + p, len + p);
                    if (n < 0) {
                        printf("Error write(): %s(%d)\n", strerror(errno), errno);
                        return;
                    } else {
                        p += n;
                    }
                }
                if (len != 4096) break;
            }
            close(connfd);
            close(filefd);
            mode = THREAD_MODE_NON;
            filefd = -1;
            fclose(f);
            break;
        case RETR:
            if (mode == THREAD_MODE_PASV)
                connfd = filefd;
            else {
                struct sockaddr addr;
                socklen_t len;
                connfd = accept(filefd, &addr, &len);
                if (connfd == -1) {
                    printf("accept error\n");
                    return;
                }
            }
            f = fopen(filename, "w");
            if (f == NULL) {
                printf("open error\n");
                return;
            }
            while (1) {
                len = read(connfd, buff, sizeof buff);
                if (len == -1) {
                    printf("read error\n");
                    break;
                } else if (len == 0) {
                    printf("finish reading\r\n");
                    break;
                }
                fwrite(buff, len, 1, f);
            }
            close(connfd);
            close(filefd);
            mode = THREAD_MODE_NON;
            filefd = -1;
            fclose(f);
            break;
        case LIST:
            if (mode == THREAD_MODE_PASV)
                connfd = filefd;
            else {
                struct sockaddr addr;
                socklen_t len;
                connfd = accept(filefd, &addr, &len);
                if (connfd == -1) {
                    printf("accept error\n");
                    return;
                }
            }
            while (1) {
                len = read(connfd, buff, sizeof buff);
                if (len == -1) {
                    printf("read error\n");
                    break;
                }
                else if (len == 0) {
                    printf("finish reading\r\n");
                    break;
                }
                fwrite(buff, len, 1, stderr);
            }
            close(connfd);
            close(filefd);
            mode = THREAD_MODE_NON;
            filefd = -1;
            break;
        default:
            break;
    }
}
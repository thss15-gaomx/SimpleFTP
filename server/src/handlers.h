//
// Created by gmx15 on 2017/10/28.
//
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>

#include "params.h"
#include "utils.h"

void USER(Client* client, char* param) {
    if (client->status == STATUS_INIT){
        if (strcmp(param, "anonymous") == 0) {
            if (writeResponse(client, S331) == 0)
                client->status = STATUS_USER;
        } else writeResponse(client, S530P);
    }
}

void PASS(Client* client) {
    if (client->status == STATUS_USER){
        if (writeResponse(client, S230) == 0)
            client->status = STATUS_PASS;
    }
}

void SYST(Client* client) {
    writeResponse(client, S215);
}

void TYPE(Client* client) {
    writeResponse(client, S200T);
}

void PORT(Client* client, char* param) {
    int h1, h2, h3, h4, p1, p2;
    sscanf(param, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    sprintf(client->host, "%d.%d.%d.%d", h1, h2, h3, h4);
    if(client->filefd != -1)
        close(client->filefd);
    writeResponse(client, S200P);
    client->mode = THREAD_MODE_PORT;
    client->port = p1 * 256 + p2;
}

void PASV(Client* client) {
    int sockfd;
    char response[100];
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    for (int i = 20000; i <= 65535; i++) {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(i);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            if (listen(sockfd, 10) == -1) {
                printf("Error listen(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            sprintf(response, "%s(%s,%d,%d)\r\n", S227, getHost(), i / 256, i % 256);
            if (writeResponse(client, response) == -1) {
                close(sockfd);
                return;
            }
            if (client->filefd != -1)
                close (client->filefd);
            client->filefd = sockfd;
            client->mode = THREAD_MODE_PASV;
            break;
        }
    }
}

void STOR(Client* client, char* param) {
    if (client->mode == THREAD_MODE_NON) {
        writeResponse(client, S425);
        return;
    }
    char file[150];
    if (strcmp(client->path, "/") == 0)
        sprintf(file, "%s%s%s", client->root, client->path, parseFilename(param));
    else
        sprintf(file, "%s%s/%s", client->root, client->path, parseFilename(param));
    FILE* fin = fopen(file, "w");
    if (fin == NULL) {
        printf("Error fopen(), filename %s\n", file);
        writeResponse(client, S451);
        return;
    }
    if (client->mode == THREAD_MODE_PORT) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(client->port);
        if ((client->transfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (inet_pton(AF_INET, client->host, &addr.sin_addr) != 1) {
            printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (connect(client->transfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (writeResponse(client, S150) == -1 ) return;
    } else {
        struct sockaddr addr;
        socklen_t len;
        if ((client->transfd = accept(client->filefd, &addr, &len)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (writeResponse(client, S150) == -1 ) return;
    }
    int count;
    char buf[8192];
    while (1) {
        count = read(client->transfd, buf, sizeof buf);
        if (count == -1) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            break;
        }
        else if (count == 0) {
            printf("Complete reading\n");
            break;
        }
        fwrite(buf, count, 1, fin);
    }
    fclose(fin);
    close(client->transfd);
    close(client->filefd);
    client->transfd = -1;
    client->filefd = -1;
    client->mode = THREAD_MODE_NON;
    if (writeResponse(client, S226S) == -1 ) return;
}

void RETR(Client* client, char* param) {
    if (client->mode == THREAD_MODE_NON) {
        if (write(client->connfd, S425, strlen(S425)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    char file[150];
    if (strcmp(client->path, "/") == 0)
        sprintf(file, "%s%s%s", client->root, client->path, param);
    else
        sprintf(file, "%s%s/%s", client->root, client->path, param);
    FILE* fin = fopen(file, "r");
    if (fin == NULL) {
        printf("Error fopen(), filename %s\n", file);
        if (write(client->connfd, S451, strlen(S451)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    if (client->mode == THREAD_MODE_PORT) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(client->port);
        if ((client->transfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (inet_pton(AF_INET, client->host, &addr.sin_addr) != 1) {
            printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (connect(client->transfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (writeResponse(client, S150) == -1 ) return;
    } else {
        struct sockaddr addr;
        socklen_t len;
        if ((client->transfd = accept(client->filefd, &addr, &len)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (writeResponse(client, S150) == -1 ) return;
    }
    int count, p;
    char buf[8192];
    while (1) {
        count = fread(buf, 1, 8192, fin);
        if (count == 0) break;
        p = 0;
        while (p < count) {
            int n = write(client->transfd, buf + p, count - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            else p += n;
        }
        if (count != 8192) break;
    }
    fclose(fin);
    close(client->transfd);
    close(client->filefd);
    client->transfd = -1;
    client->filefd = -1;
    client->mode = THREAD_MODE_NON;
    if (writeResponse(client, S226R) == -1 ) return;
}

void QUIT(Client* client) {
    if (write(client->connfd, S221, strlen(S221)) != -1) {
        printf("Closed connection on descriptor %d\n", client->connfd);
        client->status = STATUS_QUIT;
    }
}

void PWD(Client* client) {
    char response[100];
    sprintf(response, "path: \"%s\"\r\n", client->path);
    writeResponse(client, response);
}

void MKD(Client* client, char* param) {
    char currentPath[100], destPath[100], cmd[100];
    getcwd(currentPath, 100);
    if (param[0] == '/')
        sprintf(destPath, "%s%s", client->root, param);
    else {
        if (strcmp(client->path, "/") == 0)
            sprintf(destPath, "%s/%s", client->root, param);
        else
            sprintf(destPath, "%s%s/%s", client->root, client->path, param);
    }
    sprintf(cmd, "mkdir %s", destPath);
    system(cmd);
    if (chdir(destPath) == 0) {
        printf("path: %s\n", client->path);
        chdir(currentPath);
        writeResponse(client, S250);
    } else {
        printf("no path: %s\n", destPath);
        chdir(currentPath);
        writeResponse(client, S550);
    }
}

void CWD(Client* client, char* param) {
    char originPath[100], destPath[100];
    getcwd(originPath, 100);
    if (param[0] == '/')
        sprintf(destPath, "%s%s", client->root, param);
    else {
        if (strcmp(client->path, "/") == 0)
            sprintf(destPath, "%s/%s", client->root, param);
        else
            sprintf(destPath, "%s%s/%s", client->root, client->path, param);
    }
    if (chdir(destPath) == 0) {
        strcpy(client->path, destPath + strlen(client->root));
        if (strlen(client->path) == 0)
            strcpy(client->path, "/");
        printf("path: %s\n", client->path);
        chdir(originPath);
        writeResponse(client, S250);
    }
    else {
        printf("no path: %s\n", destPath);
        writeResponse(client, S550C);
    }
}

void RMD(Client* client, char* param) {
    char originPath[100], destPath[100], cmd[100];
    getcwd(originPath, 100);
    if (param[0] == '/') {
        sprintf(destPath, "%s%s", client->root, param);
    }
    else {
        if (strcmp(client->path, "/") == 0)
            sprintf(destPath, "%s/%s", client->root, param);
        else
            sprintf(destPath, "%s%s/%s", client->root, client->path, param);
    }
    if (chdir(destPath) == 0) {
        sprintf(cmd, "rm -r %s", destPath);
        system(cmd);
        printf("path: %s\n", client->path);
        chdir(originPath);
        writeResponse(client, S250);
    }
    else {
        printf("no path: %s\n", destPath);
        chdir(originPath);
        writeResponse(client, S550);
    }
}

void LIST(Client* client) {
    if (client->mode == THREAD_MODE_NON) {
        writeResponse(client, S425);
        return;
    } else if (client->mode == THREAD_MODE_PORT) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(client->port);
        if ((client->transfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (inet_pton(AF_INET, client->host, &addr.sin_addr) != 1) {
            printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (connect(client->transfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (writeResponse(client, S150) == -1) return;
    } else {
        if ((client->transfd = accept(client->filefd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (writeResponse(client, S150) == -1) return;
    }
    char cmd[200];
    sprintf(cmd, "ls -l %s%s", client->root, client->path);
    FILE *fname = popen(cmd, "r");
    int count, p;
    char buf[4096];
    while (1) {
        count = fread(buf, 1, 4096, fname);
        if (count == 0) break;
        p = 0;
        while (p < count) {
            int n = write(client->transfd, buf + p, count - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            else p += n;
        }
        if (count != 4096) break;
    }
    pclose(fname);
    close(client->transfd);
    close(client->filefd);
    client->mode = THREAD_MODE_NON;
    client->transfd = -1;
    client->filefd = -1;
    if (writeResponse(client, S226L) == -1) return;
}

void DELE(Client* client, char* param) {
    char destPath[100];
    if (param[0] == '/') {
        sprintf(destPath, "%s%s", client->root, param);
    }
    else {
        if (strcmp(client->path, "/") == 0)
            sprintf(destPath, "%s/%s", client->root, param);
        else
            sprintf(destPath, "%s%s/%s", client->root, client->path, param);
    }
    FILE *fin = fopen(destPath, "r");
    if (fin == NULL) {
        if (writeResponse(client, S550) == -1) return;
    }
    else {
        fclose(fin);
        char cmd[100];
        sprintf(cmd, "rm %s", destPath);
        system(cmd);
        printf("delete %s\n", destPath);
        if (writeResponse(client, S250) == -1) return;
    }
}

void RNFR(Client* client, char* param) {
    char destPath[100];
    if (param[0] == '/') {
        sprintf(destPath, "%s%s", client->root, param);
    }
    else {
        if (strcmp(client->path, "/") == 0)
            sprintf(destPath, "%s/%s", client->root, param);
        else
            sprintf(destPath, "%s%s/%s", client->root, client->path, param);
    }
    FILE *fin = fopen(destPath, "r");
    if (fin == NULL) {
        if (writeResponse(client, S550) == -1) return;
    }
    else {
        fclose(fin);
        strcpy(client->rename_path, destPath);
        if (writeResponse(client, S350) == -1) return;
    }
}

void RNTO(Client* client, char* param) {
    char destPath[100];
    if (param[0] == '/') {
        sprintf(destPath, "%s%s", client->root, param);
    }
    else {
        if (strcmp(client->path, "/") == 0)
            sprintf(destPath, "%s/%s", client->root, param);
        else
            sprintf(destPath, "%s%s/%s", client->root, client->path, param);
    }
    char cmd[100];
    sprintf(cmd, "mv %s %s", client->rename_path, destPath);
    system(cmd);
    FILE *fin = fopen(destPath, "r");
    if (fin == NULL) {
        if (writeResponse(client, S550) == -1) return;
    }
    else {
        fclose(fin);
        strcpy(client->rename_path, destPath);
        if (writeResponse(client, S250) == -1) return;
    }
}

void clientHandler(Client* client, char* sentence) {
    if (judgeCmd(sentence, "USER"))
        USER(client, parseCmd(sentence, "USER"));
    else if (judgeCmd(sentence, "PASS")) {
        if (client->status == STATUS_USER)
            PASS(client);
        else
            writeResponse(client, S503);
    }
    else if (judgeCmd(sentence, "SYST")) {
        if (client->status == STATUS_PASS)
            SYST(client);
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "TYPE")) {
        if (client->status == STATUS_PASS)
            TYPE(client);
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "PORT")) {
        if (client->status == STATUS_PASS)
            PORT(client, parseCmd(sentence, "PORT"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "PASV")) {
        if (client->status == STATUS_PASS)
            PASV(client);
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "STOR")) {
        if (client->status == STATUS_PASS)
            STOR(client, parseCmd(sentence, "STOR"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "RETR")) {
        if (client->status == STATUS_PASS)
            RETR(client, parseCmd(sentence, "RETR"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "PWD")) {
        if (client->status == STATUS_PASS)
            PWD(client);
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "MKD")) {
        if (client->status == STATUS_PASS)
            MKD(client, parseCmd(sentence, "MKD"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "CWD")) {
        if (client->status == STATUS_PASS)
            CWD(client, parseCmd(sentence, "CWD"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "RMD")) {
        if (client->status == STATUS_PASS)
            RMD(client, parseCmd(sentence, "RMD"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "LIST")) {
        if (client->status == STATUS_PASS)
            LIST(client);
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "DELE")) {
        if (client->status == STATUS_PASS)
            DELE(client, parseCmd(sentence, "DELE"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "RNFR")) {
        if (client->status == STATUS_PASS)
            RNFR(client, parseCmd(sentence, "RNFR"));
        else
            writeResponse(client, S530);
    }
    else if (judgeCmd(sentence, "RNTO")) {
        if (client->status == STATUS_PASS)
            RNTO(client, parseCmd(sentence, "RNTO"));
        else
            writeResponse(client, S530);
    }
    else
        QUIT(client);
}
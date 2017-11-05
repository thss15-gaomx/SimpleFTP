//
// Created by gmx15 on 2017/10/28.
//
#include <string.h>
#include <stdlib.h>
#include "params.h"

void initClient(Client* client){
    client->status = STATUS_INIT;
    client->filefd = -1;
    client->transfd = -1;
    client->mode = THREAD_MODE_NON;
    client->port = -1;
    client->connfd = -1;
    strcpy(client->root, defaultRoot);
    strcpy(client->path, "/");
}

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

char* parseCmd(char* sentence, char* cmd) {
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
    return param;
}

char* parseFilename(char* param) {
    char* filename = (char*)malloc((sizeof(char)) * strlen(param));
    for (int i = strlen(param) - 1; i >= -1; i--) {
        if (param[i] == '/' || i == -1) {
            strcpy(filename, param + i + 1);
            break;
        }
    }
    return filename;
}

int writeResponse(Client* client, char* S) {
    if (write(client->connfd, S, strlen(S)) == -1) {
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

char* getHost() {
    struct ifaddrs *head = NULL, *iter = NULL;
    if (getifaddrs(&head) == -1) return NULL;
    for (iter = head; iter != NULL; iter = iter->ifa_next) {
        if (iter->ifa_addr == NULL) continue;
        if (iter->ifa_addr->sa_family != AF_INET) continue;
        char mask[INET_ADDRSTRLEN];
        void* ptr = &((struct sockaddr_in*) iter->ifa_netmask)->sin_addr;
        inet_ntop(AF_INET, ptr, mask, INET_ADDRSTRLEN);
        if (strcmp(mask, "255.0.0.0") == 0) continue;
        void* src = &((struct sockaddr_in *) iter->ifa_addr)->sin_addr;
        char* dst = (char*)malloc(20);
        memset(dst, 0, 20);
        inet_ntop(AF_INET, src, dst, INET_ADDRSTRLEN);
        int len = strlen(dst);
        for(int i = 0; i < len; i++) {
            if (dst[i] == '.')
                dst[i] = ',';
        }
        return dst;
    }
    return NULL;
}
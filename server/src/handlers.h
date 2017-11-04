//
// Created by gmx15 on 2017/10/28.
//
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include "params.h"
#include "utils.h"

void USER(int connfd) {
    if (status == 0){
        if (write(connfd, S331, strlen(S331)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        else
            status = 1;
    }
}

void PASS(int connfd) {
    if (status == 1){
        if (write(connfd, S230, strlen(S230)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        else
            status = 2;
    }
}

void SYST(int connfd) {
    if (write(connfd, S215, strlen(S215)) == -1)
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
}

void TYPE(int connfd) {
    if (write(connfd, S200T, strlen(S200T)) == -1)
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
}

void PORT(int connfd, char* param) {
    int h1, h2, h3, h4, p1, p2;
    sscanf(param, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    sprintf(host, "%d.%d.%d.%d", h1, h2, h3, h4);
    if(filefd != -1)
        close(filefd);
    if (write(connfd, S200P, strlen(S200P)) == -1)
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
    mode = THREAD_MODE_PORT;
    port = p1 * 256 + p2;
}

void PASV(int connfd) {
    int tempfd;
    char response[100];
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    if ((tempfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    for (int i = 20000; i <= 65535; i++) {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(i);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(tempfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            printf("New connection at port %d\n", i);
            if (listen(tempfd, 10) == -1) {
                printf("Error listen(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            printf("Listen on socket %d\n", tempfd);
            sprintf(response, "%s(127,0,0,1,%d,%d)\r\n", S227, i / 256, i % 256);
            if (write(connfd, response, strlen(response)) == -1) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                close(tempfd);
                return;
            }
            if (filefd != -1)
                close (filefd);
            filefd = tempfd;
            mode = THREAD_MODE_PASV;
            break;
        }
    }
}

void STOR(int connfd, char* param) {
    if (mode == THREAD_MODE_NON) {
        if (write(connfd, S425, strlen(S425)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    char file[150];
    if (strcmp(path, "/") == 0)
        sprintf(file, "%s%s%s", root, path, parseFilename(param));
    else
        sprintf(file, "%s%s/%s", root, path, parseFilename(param));
    FILE* fin = fopen(file, "w");
    if (fin == NULL) {
        printf("Error fopen(), filename %s\n", file);
        if (write(connfd, S451, strlen(S451)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    int sockfd;
    if (mode == THREAD_MODE_PORT) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if ((filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
            printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (connect(filefd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (write(connfd, S150, strlen(S150)) == -1) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        sockfd = filefd;
    } else if (mode == THREAD_MODE_PASV) {
        struct sockaddr addr;
        socklen_t len;
        if ((sockfd = accept(filefd, &addr, &len)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (write(connfd, S150, strlen(S150)) == -1) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return;
        }
    }
    int count;
    char buf[8192];
    while (1) {
        count = read(sockfd, buf, sizeof buf);
        if (count == -1) {
            printf("read error\n");
            break;
        }
        else if (count == 0) {
            printf("finish reading\n");
            break;
        }
        fwrite(buf, count, 1, fin);
    }
//    do {
//        count = read(sockfd, buf, 8192);
//        fwrite(buf, count, 1, fin);
//    } while (count > 0);
    fclose(fin);
    close(sockfd);
    close(filefd);
    filefd = -1;
    mode = THREAD_MODE_NON;
    if (write(connfd, S226S, strlen(S226S)) == -1) {
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
}

void RETR(int connfd, char* param) {
    if (mode == THREAD_MODE_NON) {
        if (write(connfd, S425, strlen(S425)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    char file[150];
    if (strcmp(path, "/") == 0)
        sprintf(file, "%s%s%s", root, path, param);
    else
        sprintf(file, "%s%s/%s", root, path, param);
    FILE* fin = fopen(file, "r");
    if (fin == NULL) {
        printf("Error fopen(), filename %s\n", file);
        if (write(connfd, S451, strlen(S451)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
    int sockfd;
    if (mode == THREAD_MODE_PORT) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if ((filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
            printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (connect(filefd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (write(connfd, S150, strlen(S150)) == -1) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        sockfd = filefd;
    } else if (mode == THREAD_MODE_PASV) {
        struct sockaddr addr;
        socklen_t len;
        sockfd = accept(filefd, &addr, &len);
        if (sockfd == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (write(connfd, S150, strlen(S150)) == -1) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return;
        }
    }
    int count;
    char buf[8192];
//    do {
//        count = fread(buf, 1, 8192, fin);
//        write(sockfd, buf, count);
//    } while (count > 0);
    int p;
    while (1) {
        count = fread(buf, 1, 8192, fin);
        if (count == 0) break;
        p = 0;
        while (p < count) {
            int n = write(sockfd, buf + p, count - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            else p += n;
        }
        if (count != 8192) break;
    }
    fclose(fin);
    close(sockfd);
    close(filefd);
    filefd = -1;
    mode = THREAD_MODE_NON;
    if (write(connfd, S226R, strlen(S226R)) == -1) {
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
}

void QUIT(int connfd) {
    if (write(connfd, S221, strlen(S221)) != -1) {
        printf("Closed connection on descriptor %d\n", connfd);
        status = STATUS_QUIT;
    }
}

void PWD(int connfd) {
    char msg[100];
    sprintf(msg, "path: \"%s\"\r\n", path);
    if (write(connfd, msg, strlen(msg)) == -1)
        printf("write wrong\n");
}

void MKD(int connfd, char* param) {
    char currentPath[100], cpath[100], cmd[100];
    getcwd(currentPath, 100);
    if (param[0] == '/')
        sprintf(cpath, "%s%s", root, param);
    else {
        if (strcmp(path, "/") == 0)
            sprintf(cpath, "%s/%s", root, param);
        else
            sprintf(cpath, "%s%s/%s", root, path, param);
    }
    sprintf(cmd, "mkdir %s", cpath);
    system(cmd);
    if (chdir(cpath) == 0) {
        printf("path: %s\n", path);
        chdir(currentPath);
        if (write(connfd, S250, strlen(S250)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
    } else {
        printf("no path: %s\n", cpath);
        chdir(currentPath);
        if (write(connfd, S550, strlen(S550)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
    }
}

void CWD(int connfd, char* param) {
    char originPath[100], cpath[100];
    getcwd(originPath, 100);
    if (param[0] == '/')
        sprintf(cpath, "%s%s", root, param);
    else {
        if (strcmp(path, "/") == 0)
            sprintf(cpath, "%s/%s", root, param);
        else
            sprintf(cpath, "%s%s/%s", root, path, param);
    }
    if (chdir(cpath) == 0) {
        strcpy(path, cpath + strlen(root));
        if (strlen(path) == 0)
            strcpy(path, "/");
        printf("path: %s\n", path);
        chdir(originPath);
        if (write(connfd, S250, strlen(S250)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
    }
    else {
        printf("no path: %s\n", cpath);
        if (write(connfd, S550C, strlen(S550C)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
    }
}

void RMD(int connfd, char* param) {
    char originPath[100], cpath[100], cmd[100];
    getcwd(originPath, 100);
    if (param[0] == '/') {
        sprintf(cpath, "%s%s", root, param);
    }
    else {
        if (strcmp(path, "/") == 0)
            sprintf(cpath, "%s/%s", root, param);
        else
            sprintf(cpath, "%s%s/%s", root, path, param);
    }
    if (chdir(cpath) == 0) {
        sprintf(cmd, "rm -r %s", cpath);
        system(cmd);
        printf("path: %s\n", path);
        chdir(originPath);
        if (write(connfd, S250, strlen(S250)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
    }
    else {
        printf("no path: %s\n", cpath);
        chdir(originPath);
        if (write(connfd, S550, strlen(S550)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
    }
}

void LIST(int connfd) {
    int sockfd;
    if (mode == THREAD_MODE_NON) {
        if (write(connfd, S425, strlen(S425)) == -1)
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    } else if (mode == THREAD_MODE_PORT) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if ((filefd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
            printf("Error socket(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
            printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (connect(filefd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            printf("Error connect(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        if (write(connfd, S150, strlen(S150)) == -1) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return;
        }
        sockfd = filefd;
    } else if (mode == THREAD_MODE_PASV) {
        sockfd = accept(filefd, NULL, NULL);
        if (sockfd == -1) {
            printf("accept error\n");
            return;
        }
        if (write(connfd, S150, strlen(S150)) == -1) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return;
        }
    }
    char cmd[200], buf[4096];
    sprintf(cmd, "ls -l %s%s", root, path);
    FILE *fname = popen(cmd, "r");
    int count, p;
    while (1) {
        count = fread(buf, 1, 4096, fname);
        if (count == 0) break;
        p = 0;
        while (p < count) {
            int n = write(sockfd, buf + p, count - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                return;
            }
            else p += n;
        }
        if (count != 4096) break;
    }
    pclose(fname);
    close(sockfd);
    close(filefd);
    mode = THREAD_MODE_NON;
    filefd = -1;
    if (write(connfd, S226L, strlen(S226L)) == -1) {
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return;
    }
}

void clientHandler(int connfd, char* sentence) {
    if (judgeCmd(sentence, "USER"))
        USER(connfd);
    else if (judgeCmd(sentence, "PASS"))
        PASS(connfd);
    else if (judgeCmd(sentence, "SYST"))
        SYST(connfd);
    else if (judgeCmd(sentence, "TYPE"))
        TYPE(connfd);
    else if (judgeCmd(sentence, "PORT"))
        PORT(connfd, parseCmd(sentence, "PORT"));
    else if (judgeCmd(sentence, "PASV"))
        PASV(connfd);
    else if (judgeCmd(sentence, "STOR"))
        STOR(connfd, parseCmd(sentence, "STOR"));
    else if (judgeCmd(sentence, "RETR"))
        RETR(connfd, parseCmd(sentence, "RETR"));
    else if (judgeCmd(sentence, "PWD"))
        PWD(connfd);
    else if (judgeCmd(sentence, "MKD"))
        MKD(connfd, parseCmd(sentence, "MKD"));
    else if (judgeCmd(sentence, "CWD"))
        CWD(connfd, parseCmd(sentence, "CWD"));
    else if (judgeCmd(sentence, "RMD"))
        RMD(connfd, parseCmd(sentence, "RMD"));
    else if (judgeCmd(sentence, "LIST"))
        LIST(connfd);
    else
        QUIT(connfd);
}
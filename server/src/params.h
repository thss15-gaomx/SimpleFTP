//
// Created by gmx15 on 2017/10/29.
//

#pragma once

char defaultRoot[100];
int defaultPort;

int listenfd;

typedef struct{
    int connfd;
    int filefd;
    int transfd;
    int status;
    int mode;
    int port;
    char host[50];
    char root[100];
    char path[100];
    char rename_path[100];
}Client;

#define THREAD_MODE_NON 0
#define THREAD_MODE_PORT 1
#define THREAD_MODE_PASV 2

#define STATUS_QUIT -1
#define STATUS_INIT 0
#define STATUS_USER 1
#define STATUS_PASS 2

char* S150 = "150 \r\n";
char* S200 = "200 \r\n";
char* S200P = "200 Port mode established.\r\n";
char* S200T = "200 Type set to I.\r\n";
char* S220 = "220 Anonymous FTP server ready.\r\n";
char* S226S = "226 Store successful.\r\n";
char* S226R = "226 Retrive successful.\r\n";
char* S226L = "226 List finished.\r\n";
char* S227 = "227 Entering Passive Mode ";
char* S250 = "250 Okay\r\n";
char* S331 = "331 Guest login ok, send your complete e-mail address as password.\r\n";
char* S350 = "350 File exists, ready for destination name.\r\n";
char* S230 = "230 Guest login ok, access restrictions apply.\r\n";
char* S215 = "215 UNIX Type: L8\r\n";
char* S221 = "221 Bye!\r\n";
char* S425 = "425 Use PORT or PASV first.\r\n";
char* S451 = "451 Cannot open file.\r\n";
char* S503 = "503 Login with USER first.\r\n";
char* S530 = "530 Please login with USER and PASS.\r\n";
char* S530P = "530 Permission denied.\r\n";
char* S550 = "550 Failed\r\n";
char* S550C = "550 no such file or directory\r\n";

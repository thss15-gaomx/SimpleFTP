//
// Created by gmx15 on 2017/11/1.
//

#pragma once

int defaultPort;
int port;
char host[100];
int status, cmd, mode;
int filefd;
char filename[100];

#define STATUS_START 0
#define STATUS_CMD 1
#define STATUS_PROCESS 2

#define THREAD_MODE_NON 0
#define THREAD_MODE_PORT 1
#define THREAD_MODE_PASV 2

#define MODE_NONE -2
#define ERROR -1
#define WRONG_CMD 0
#define USER 1
#define PASS 2
#define SYST 3
#define TYPE 4
#define PORT 5
#define PASV 6
#define STOR 7
#define RETR 8
#define PWD 9
#define MKD 10
#define CWD 11
#define RMD 12
#define LIST 13
#define DELE 14
#define QUIT 15
#define RNFR 16
#define RNTO 17
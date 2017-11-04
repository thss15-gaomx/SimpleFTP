//
// Created by gmx15 on 2017/10/28.
//
#include <string.h>
#include <stdlib.h>

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

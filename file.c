#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"

char* get_source(char* path, size_t* file_size) {
    if(path == NULL) {
        fprintf(stderr, "Path is NULL !\n");
        return NULL;
    }
    FILE* hdl_source = fopen(path, "r");
    if(hdl_source == NULL) {
        fprintf(stderr, "Failed to open file !\n");
        return NULL;
    }
    fseek(hdl_source, 0L, SEEK_END);
    size_t size = (size_t)ftell(hdl_source);
    rewind(hdl_source);
    
    
    char* source = malloc(size);
    if(source == NULL) {
        fprintf(stderr, "Memory allocation for the file data buffer has failed !\n");
        return NULL;
    }
    fread(source, sizeof(char), size, hdl_source);
    fclose(hdl_source);
    if(file_size != NULL) {
        *file_size = size;
    }
    return source;
}

void get_hdl_path(char* path) {

    printf("What is the dir path of the Dynamatic output (hdl) ?\n");
    char temp[MAX_NAME_LENGTH];
    char* result = fgets(temp, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    if(temp[0] != '/') {
        char* result = getcwd(path, MAX_NAME_LENGTH);
        if(result == NULL) {
            fprintf(stderr, "getcwd error !\n");
            return;
        }
        if(temp[0] != '.') {
            path[strlen(path)] = '/';
            if(strlen(path) + strlen(temp) + 1 >= MAX_NAME_LENGTH) {
                fprintf(stderr, "Path too long !\n");
            }
            strcpy(path + strlen(path), temp);
            path[strlen(path) - 1] = '\0';
        }
        else {
            path[strlen(path)] = '\0';
        }
    }
    else {
        if(strlen(temp) + 1 >= MAX_NAME_LENGTH) {
            fprintf(stderr, "Path too long !\n");
        }
        strcpy(path, temp);
        path[strlen(path) - 1] = '\0';
    }
    return;
}

void get_hdl_name(char* path) {

    printf("What is the name of the top file ?\n");
    char* result = fgets(path, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    path[strlen(path) - 1] = '\0';
    return;
}
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
    
    
    char* source = malloc(size + 1);
    if(source == NULL) {
        fprintf(stderr, "Memory allocation for the file data buffer has failed !\n");
        return NULL;
    }
    fread(source, sizeof(char), size, hdl_source);
    fclose(hdl_source);
    if(file_size != NULL) {
        *file_size = size;
    }
    source[size] = '\0';
    return source;
}

error_t get_hdl_path(char* path) {
    CHECK_PARAM(path);

    printf("What is the dir path of the Dynamatic output (hdl) ?\n");
    char temp[MAX_NAME_LENGTH];
    char* result = fgets(temp, MAX_NAME_LENGTH, stdin);
    CHECK_COND(result == NULL, ERR_FILE, "There was an error while reading the input !");
    if(temp[0] != '/') {
        char* result = getcwd(path, MAX_NAME_LENGTH);
        CHECK_COND(result == NULL, ERR_FILE, "getcwd error !");
        if(temp[0] != '.') {
            CHECK_LENGTH(strlen(path) + strlen(temp) + 2, MAX_NAME_LENGTH);
            if(path[strlen(path) - 1] == '/') {
                path[strlen(path) - 1] = '\0';
            }  
            path[strlen(path)] = '/';
            strcpy(path + strlen(path), temp);
            path[strlen(path) - 1] = '\0';
        }
    }
    else {
        CHECK_LENGTH(strlen(temp) + 1, MAX_NAME_LENGTH);
        strcpy(path, temp);
        path[strlen(path) - 1] = '\0';
    }
    if(path[strlen(path) - 1] == '/') {
        path[strlen(path) - 1] = '\0';
    }  
    return ERR_NONE;
}

error_t get_hdl_name(char* path) {
    CHECK_PARAM(path);

    printf("What is the name of the top file ?\n");
    char* result = fgets(path, MAX_NAME_LENGTH, stdin);
    CHECK_COND(result == NULL, ERR_FILE, "There was an error while reading the input !");
    path[strlen(path) - 1] = '\0';
    return ERR_NONE;
}
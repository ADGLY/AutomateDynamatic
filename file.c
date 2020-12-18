#include "file.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char* get_source(char* path) {
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
    size_t file_size = (size_t)ftell(hdl_source);
    rewind(hdl_source);
    
    
    char* source = malloc(file_size);
    if(source == NULL) {
        fprintf(stderr, "Memory allocation for the file data buffer has failed !\n");
        return NULL;
    }
    fread(source, sizeof(char), file_size, hdl_source);
    fclose(hdl_source);
    return source;
}

void get_hdl_path(char* path) {
    printf("Where is top hdl file located ?\n");
    char* result = fgets(path, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    path[strlen(path) - 1] = '\0';
    return;
}
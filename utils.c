#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "utils.h"

char* get_source(const char* path, size_t* file_size) {
    if(path == NULL) {
        fprintf(stderr, "Path is NULL !\n");
        return NULL;
    }
    FILE* file = fopen(path, "r");
    if(file == NULL) {
        fprintf(stderr, "Failed to open file !\n");
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size_t size = (size_t)ftell(file);
    rewind(file);
    
    
    char* source = (char*)malloc(size + 1);
    if(source == NULL) {
        fprintf(stderr, "Memory allocation for the file data buffer has failed !\n");
        fclose(file);
        return NULL;
    }
    fread(source, sizeof(char), size, file);
    fclose(file);
    if(file_size != NULL) {
        *file_size = size;
    }
    source[size] = '\0';
    return source;
}

auto_error_t get_string(char* name, const char* msg, int length) {
    CHECK_PARAM(name);
    CHECK_PARAM(msg);

    printf("%s\n", msg);
    char* result = fgets(name, length, stdin);
    CHECK_COND(result == NULL, ERR_FILE, "There was an error while reading the input !");
    name[strlen(name) - 1] = '\0';
    return ERR_NONE;
}

auto_error_t get_name(char* name, const char* msg) {
    CHECK_CALL(get_string(name, msg, MAX_NAME_LENGTH), "get_string failed !");
    return ERR_NONE;
}

auto_error_t get_path(char* path, const char* msg) {
    char temp[MAX_PATH_LENGTH];
    CHECK_CALL(get_string(temp, msg, MAX_PATH_LENGTH), "get_string failed !");
    char* final_path = realpath(temp, path);
    CHECK_COND(final_path == NULL && errno != ENOENT, ERR_PATH, "Could not resolve absolute path !");
    return ERR_NONE;
}
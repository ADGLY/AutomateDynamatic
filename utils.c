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

void free_str_arr(char** str_arr, uint8_t last) {
    if(str_arr == NULL) {
        return;
    }
    for(uint8_t i = 0; i < last; ++i) {
        if(str_arr[i] != NULL) {
            free(str_arr[i]);
        }
    }
    free(str_arr);
}

auto_error_t allocate_str_arr(char*** str_arr, uint8_t* last) {
    CHECK_PARAM(str_arr);
    CHECK_PARAM(last);
    char** new_str_arr;
    uint8_t prev_last = *last;
    if(*last == 0) {
        new_str_arr = calloc(5, sizeof(char*));
        *last = 5;
    }
    else {
        new_str_arr = realloc(*str_arr, *last * 2 * sizeof(char*));
        *last = *last * 2;
    }
    CHECK_COND_DO(new_str_arr == NULL, ERR_MEM, "Could not realloc for str_arr !", free_str_arr(*str_arr, prev_last););
    *str_arr = new_str_arr;

    for(uint8_t i = prev_last; i < *last; ++i) {
        (*str_arr)[i] = calloc(MAX_PATH_LENGTH, sizeof(char));
        CHECK_COND_DO((*str_arr)[i] == NULL, ERR_MEM, "Could not realloc for str_arr !", free_str_arr(*str_arr, i););
    }
    return ERR_NONE;
}
#pragma once

#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include "error.h"

#define MAX_NAME_LENGTH NAME_MAX
#define MAX_PATH_LENGTH PATH_MAX

#define GET_SUFFIX(arr, str)            \
    const char* str;                    \
    if((arr)->write && (arr)->read) {   \
        str = "read_write";             \
    }                                   \
    else if((arr)->write) {             \
        str = "write";                  \
    }                                   \
    else if((arr)->read) {              \
        str = "read";                   \
    }

char* get_source(const char* path, size_t* file_size);
void str_toupper(char* str);
auto_error_t get_name(char* name, const char* msg);
auto_error_t get_path(char* path, const char* msg);
void free_str_arr(char** str_arr, uint8_t last);
auto_error_t allocate_str_arr(char*** str_arr, uint8_t* last);
void clean_folder();
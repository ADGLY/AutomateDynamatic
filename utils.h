#pragma once

#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include "error.h"

#define MAX_NAME_LENGTH NAME_MAX
#define MAX_PATH_LENGTH PATH_MAX

char* get_source(const char* path, size_t* file_size);
auto_error_t get_name(char* name, const char* msg);
auto_error_t get_path(char* path, const char* msg);
void free_str_arr(char** str_arr, uint8_t last);
auto_error_t allocate_str_arr(char*** str_arr, uint8_t* last);
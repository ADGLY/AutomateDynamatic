#pragma once

#include <stddef.h>
#include <limits.h>
#include "error.h"

#define MAX_NAME_LENGTH NAME_MAX
#define MAX_PATH_LENGTH PATH_MAX

char* get_source(const char* path, size_t* file_size);
error_t get_name(char* name, const char* msg);
error_t get_path(char* path, const char* msg);
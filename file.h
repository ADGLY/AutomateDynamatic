#pragma once

#include <stddef.h>
#include "error.h"

#define MAX_NAME_LENGTH 256

error_t get_hdl_path(char* path, const char* exec_path);
char* get_source(const char* path, size_t* file_size);
error_t get_hdl_name(char* path);
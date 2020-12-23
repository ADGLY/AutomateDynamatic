#pragma once

#include <stddef.h>

#define MAX_NAME_LENGTH 256

void get_hdl_path(char* path);
char* get_source(char* path, size_t* file_size);
void get_hdl_name(char* path);
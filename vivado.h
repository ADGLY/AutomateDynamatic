#pragma once

#include <stddef.h>
#include <stdint.h>
#include "utils.h"
#include "hdl.h"

typedef struct
{
    char path[MAX_PATH_LENGTH];
    hdl_source_t *hdl_source;
    char name[MAX_NAME_LENGTH];
    char array_size_ind;
    uint16_t array_size;
} project_t;

auto_error_t create_project(project_t *project, hdl_source_t *hdl_source);

auto_error_t launch_script(const char *name, const char *exec_path);

auto_error_t project_free(project_t *project);
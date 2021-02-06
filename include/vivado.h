#pragma once

#include "hdl.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Contains general information about the Vivado project and global parameters
 **/
typedef struct _project {
    char path[MAX_PATH_LENGTH];
    hdl_source_t *hdl_source;
    char name[MAX_NAME_LENGTH];
    char array_size_ind;
    uint16_t array_size;
} project_t;

/**
 * Initializes a porject struct
 *
 * @param project
 * @param hdl_sources
 *
 * @return an error code
 **/
auto_error_t create_project(project_t *project, hdl_source_t *hdl_source);

/**
 * Launch Vivado and sources the script given as argument
 *
 * @param name the tcl script to source
 * @param exec_path the working directory
 *
 * @return an error code
 **/
auto_error_t launch_script(const char *name, const char *exec_path);

/**
 * Frees a project struct
 *
 * @param project the struct to free
 *
 * @return an error code
 **/
auto_error_t project_free(project_t *project);
#pragma once

#include <stddef.h>
#include "file.h"
#include "hdl.h"

typedef struct {
    const char* top_file;
    char top_file_path[MAX_NAME_LENGTH];
    const char* axi_file;
    char axi_file_path[MAX_NAME_LENGTH];
} axi_files_t;

typedef struct {
    char  name[MAX_NAME_LENGTH];
    char path[MAX_NAME_LENGTH];
    char interface_name[MAX_NAME_LENGTH];
    size_t nb_slave_registers;
    axi_files_t axi_files;
    int revision;
} axi_ip_t;
 
typedef struct {
    axi_ip_t axi_ip;
    char path[MAX_NAME_LENGTH];
    hdl_source_t* hdl_source;
    char name[MAX_NAME_LENGTH];
} project_t;

void create_project(project_t* project, hdl_source_t* hdl_source);

void launch_script(const char* name);
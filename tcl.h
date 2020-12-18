#pragma once

#include <stddef.h>
#include "file.h"
#include "hdl_data.h"

typedef struct {
    const char* top_file;
    const char* axi_file;
} axi_files_t;

typedef struct {
    char  name[MAX_NAME_LENGTH];
    char path[MAX_NAME_LENGTH];
    char interface_name[MAX_NAME_LENGTH];
    size_t nb_slave_registers;
    axi_files_t axi_files;
} axi_ip_t;
 
typedef struct {
    axi_ip_t axi_ip;
    char path[MAX_NAME_LENGTH];
    hdl_source_t* hdl_source;
} project_t;

void create_project(project_t* project, hdl_source_t* hdl_source);

void generate_AXI_script(project_t* vivado_project);

void free_axi_ip(axi_ip_t* axi_ip);
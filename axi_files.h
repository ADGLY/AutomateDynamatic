#pragma once

#include "vivado.h"

typedef struct {
    const char* top_file;
    char top_file_path[MAX_PATH_LENGTH];
    const char* axi_file;
    char axi_file_path[MAX_PATH_LENGTH];
} axi_files_t;

typedef struct {
    char  name[MAX_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
    char interface_name[MAX_NAME_LENGTH];
    size_t nb_slave_registers;
    axi_files_t axi_files;
    int revision;
} axi_ip_t;

auto_error_t create_axi(axi_ip_t* axi_ip, project_t* project);

auto_error_t read_axi_files(axi_ip_t* axi_ip);

auto_error_t update_files(project_t* project, axi_ip_t* axi_ip);

auto_error_t free_axi(axi_ip_t* axi_ip);
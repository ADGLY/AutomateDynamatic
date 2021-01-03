#pragma once

#include <stddef.h>
#include <stdint.h>
#include "file.h"
#include "hdl.h"

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

typedef struct {
    char script_path[MAX_PATH_LENGTH];
    char hdl_path[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH];
    uint8_t latency;
    size_t appearance_offset;
    char** arith_unit_name_list;
    size_t nb_arith_names;
} float_op_t;

typedef struct vivado_hls_ {
    char project_path[MAX_PATH_LENGTH];
    char* hls_source;
    char source_path[MAX_PATH_LENGTH];
    char fun_name[MAX_NAME_LENGTH];
    float_op_t* float_ops;
    uint8_t nb_float_op;
} vivado_hls_t;
 
typedef struct project_ {
    axi_ip_t axi_ip;
    char path[MAX_PATH_LENGTH];
    hdl_source_t* hdl_source;
    char name[MAX_NAME_LENGTH];
    char array_size_ind;
    uint16_t array_size;
    vivado_hls_t* hls;
} project_t;

error_t create_project(project_t* project, hdl_source_t* hdl_source, vivado_hls_t* hls);

error_t launch_script(const char* name, const char* exec_path);

error_t project_free(project_t* project);
#pragma once

#include "axi_files.h"
#include "error.h"
#include "utils.h"
#include "vivado.h"

typedef struct {
    char script_path[MAX_PATH_LENGTH];
    char hdl_path[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH];
    uint8_t latency;
    size_t appearance_offset;
    char **arith_unit_name_list;
    size_t nb_arith_names;
} float_op_t;

typedef struct vivado_hls_ {
    char project_path[MAX_PATH_LENGTH];
    char *hls_source;
    char source_path[MAX_PATH_LENGTH];
    char fun_name[MAX_NAME_LENGTH];
    float_op_t *float_ops;
    uint8_t nb_float_op;
} vivado_hls_t;

auto_error_t create_hls(vivado_hls_t *hls, hdl_source_t *hdl_source);
auto_error_t parse_hls(vivado_hls_t *hls, hdl_source_t *hdl_source);
auto_error_t launch_hls_script();
auto_error_t resolve_float_ops();
auto_error_t update_arithmetic_units(project_t *project, vivado_hls_t *hls,
                                     axi_ip_t *axi_ip);
auto_error_t hls_free(vivado_hls_t *hls);
#pragma once

#include "axi_files.h"
#include "error.h"
#include "utils.h"
#include "vivado.h"

/**
 * Contains all relevant information for a floating point operator
 **/
typedef struct _float_op {
    char script_path[MAX_PATH_LENGTH];
    char hdl_path[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH];
    uint8_t latency;
    size_t appearance_offset;
    char **arith_unit_name_list;
    size_t nb_arith_names;
} float_op_t;

/**
 * Contains all relevant information the VivadoHLS project
 **/
typedef struct vivado_hls_ {
    char project_path[MAX_PATH_LENGTH];
    char *hls_source;
    char source_path[MAX_PATH_LENGTH];
    char fun_name[MAX_NAME_LENGTH];
    float_op_t *float_ops;
    uint8_t nb_float_op;
    bool faddfsub;
} vivado_hls_t;

/**
 * Initializes a vivado_hls_t struct
 *
 * @param hls
 * @param hdl_info
 *
 * @return an error code
 **/
auto_error_t create_hls(vivado_hls_t *hls, hdl_info_t *hdl_info);

/**
 * Parses the C/C++ source file. This will, for each array, detect if is read
 * only, write only or read and write
 *
 * @param hls
 * @param hdl_info
 *
 * @return an error code
 **/
auto_error_t parse_hls(vivado_hls_t *hls, hdl_info_t *hdl_info);

/**
 * Launches VivadoHLS and source the HLS tcl script
 **/
auto_error_t launch_hls_script();

/**
 * Reads the output of VivadoHLS and retrieves informations for floating point
 *operators
 **/
auto_error_t resolve_float_ops(vivado_hls_t *hls, hdl_info_t *hdl_info);

/**
 * Updates the arithmetic_units.vhd file so that the floating point compoennts
 * used by Dynamatic are linked to the real oness
 *
 * @param project
 * @param hls
 * @param axi_ip
 *
 * @return an error code
 **/
auto_error_t update_arithmetic_units(project_t *project, vivado_hls_t *hls,
                                     axi_ip_t *axi_ip);

/**
 * Frees an hls struct
 *
 * @param hls the struct to free
 *
 * @return an error code
 **/
auto_error_t hls_free(vivado_hls_t *hls);
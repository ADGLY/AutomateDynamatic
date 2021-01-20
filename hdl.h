#pragma once

#include "error.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>

#define NB_BRAM_INTERFACE 5

typedef struct {
    char name[MAX_NAME_LENGTH];
    uint16_t width;
} hdl_in_param_t;

typedef struct {
    char address[MAX_NAME_LENGTH];
    char ce[MAX_NAME_LENGTH];
    char we[MAX_NAME_LENGTH];
    char dout[MAX_NAME_LENGTH];
    char din[MAX_NAME_LENGTH];
} bram_interface_t;

typedef struct {
    char name[MAX_NAME_LENGTH];
    bram_interface_t read_ports;
    bram_interface_t write_ports;
    bool read;
    bool write;
    uint16_t width;
} hdl_array_t;

typedef struct {
    char top_file_path[MAX_PATH_LENGTH];
    char dir[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH];
    char top_file_name[MAX_NAME_LENGTH];
    hdl_array_t *arrays;
    hdl_in_param_t *params;
    size_t nb_arrays;
    size_t nb_params;
    char *source;
    size_t end_of_ports_decl;
    size_t end_arrays;
    char exec_path[MAX_PATH_LENGTH];
    uint16_t end_out_width;
    uint16_t max_param_width;
} hdl_source_t;

auto_error_t hdl_create(hdl_source_t *hdl_source);
auto_error_t parse_hdl(hdl_source_t *hdl_source);
auto_error_t hdl_free(hdl_source_t *hdl_source);
#pragma once

#include <stddef.h>
#include "file.h"

#define NB_BRAM_INTERFACE 5

typedef struct {
    char name[MAX_NAME_LENGTH];
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
} hdl_array_t;


typedef struct {
    char top_file_path[MAX_NAME_LENGTH];
    char dir[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    hdl_array_t* arrays;
    hdl_in_param_t* params;
    size_t nb_arrays;
    size_t nb_params;
    char* source;
    size_t end_of_ports_decl;
} hdl_source_t;


void hdl_create(hdl_source_t* hdl_source);
void parse_hdl(hdl_source_t* hdl_source);
void hdl_free(hdl_source_t* hdl_source);
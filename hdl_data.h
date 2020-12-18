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
    char path[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    hdl_array_t* arrays;
    hdl_in_param_t* params;
    size_t nb_arrays;
    size_t nb_params;
    char* source;
} hdl_source_t;

static const char* const write_ports[NB_BRAM_INTERFACE] = {
    "_address0", "_ce0", "_we0", "_dout0", "_din0"
};

static const char* const read_ports[NB_BRAM_INTERFACE] = {
    "_address1", "_ce1", "_we1", "_dout1", "_din1"
};

hdl_source_t* hdl_create();
void parse_hdl(hdl_source_t* hdl_source);
void hdl_free(hdl_source_t* hdl_source);
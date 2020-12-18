#pragma once

#include "file.h"
#include "hdl_data.h"
#include <stddef.h>

typedef struct {
    char  name[MAX_NAME_LENGTH];
    char path[MAX_NAME_LENGTH];
    char interface_name[MAX_NAME_LENGTH];
    size_t nb_slave_registers;
} axi_ip_t;

void create_AXI_script(hdl_source_t* hdl_source);
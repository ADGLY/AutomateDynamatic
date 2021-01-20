#pragma once

#include "axi_files.h"
#include "vivado.h"
#include <stdio.h>

typedef void (*script_func_t)(FILE *tcl_script, project_t *project,
                              axi_ip_t *axi_ip);

script_func_t select_part_script(const char *part);
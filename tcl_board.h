#pragma once

#include <stdio.h>
#include "vivado.h"
#include "axi_files.h"

typedef void (*script_func_t)(FILE *tcl_script, project_t *project, axi_ip_t *axi_ip);

script_func_t select_part_script(const char *part);
#pragma once

#include "axi_files.h"
#include "vivado.h"
#include <stdio.h>

/**
 * Function type for board specific tcl scripts
 **/
typedef void (*script_func_t)(FILE *tcl_script, project_t *project,
                              axi_ip_t *axi_ip);

/**
 * Returns the correct script function depending on the part number.
 * Returns NULL if the part number is not recognized
 *
 * @param part
 *
 * @return an error code
 *
 **/
script_func_t select_part_script(const char *part);
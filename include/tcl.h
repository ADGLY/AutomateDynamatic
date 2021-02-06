#pragma once

#include "axi_files.h"
#include "vivado.h"
#include "vivado_hls.h"

/**
 * Generates the tcl script that will create the AXI Peripheral
 *
 * @param project will be read
 * @param axi_ip will be read
 *
 * @return an error code
 *
 **/
auto_error_t generate_AXI_script(project_t *project, axi_ip_t *axi_ip);

/**
 * Generates the tcl script that will create the base project. This script will
 *also source the AXI script.
 *
 * @param project read only
 * @param part the part number for which we want to create a project
 *
 * @return an error code
 *
 **/
auto_error_t generate_MAIN_script(project_t *project, const char *part);

/**
 * Generates the tcl script that will finalize the block design.
 * Creates the memories, add the AXI IP and make the necessary connections.
 * If the part number is recognized makes further connections.
 *
 * @param project
 * @param hls
 * @param axi_ip
 * @param part
 *
 * @return an error code
 *
 **/
auto_error_t generate_final_script(project_t *project, vivado_hls_t *hls,
                                   axi_ip_t *axi_ip, const char *part);

/**
 * Genererates the tcl script that will create the VivadoHLS project and
 *synthesize the C/C++ source file
 *
 * @param hls
 * @param part
 *
 * @return an error code
 *
 **/
auto_error_t generate_hls_script(vivado_hls_t *hls, const char *part);
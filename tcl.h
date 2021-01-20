#pragma once

#include "axi_files.h"
#include "vivado.h"
#include "vivado_hls.h"

auto_error_t generate_AXI_script(project_t *project, axi_ip_t *axi_ip);

auto_error_t generate_MAIN_script(project_t *project, const char *part);

auto_error_t generate_final_script(project_t *project, vivado_hls_t *hls,
                                   axi_ip_t *axi_ip, const char *part);

auto_error_t generate_hls_script(vivado_hls_t *hls, const char *part);
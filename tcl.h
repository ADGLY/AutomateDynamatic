#pragma once

#include "vivado.h"

error_t generate_AXI_script(project_t* vivado_project);

error_t generate_MAIN_script(project_t* project);

error_t generate_final_script(project_t* project);

error_t generate_hls_script(vivado_hls_t* hls);
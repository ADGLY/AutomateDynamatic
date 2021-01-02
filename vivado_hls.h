#pragma once

#include "error.h"
#include "file.h"
#include "vivado.h"

error_t create_hls(vivado_hls_t* hls, hdl_source_t* hdl_source);
error_t parse_hls(vivado_hls_t* hls, hdl_source_t* hdl_source);
error_t launch_hls_script();
error_t find_float_op(vivado_hls_t* hls);
error_t hls_free(vivado_hls_t* hls);
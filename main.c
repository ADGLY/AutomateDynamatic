#include <stdio.h>
#include "hdl.h"
#include "axi_files.h"
#include "tcl.h"
#include "vivado.h"
#include "vivado_hls.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

// histogram_elaborated_optimized.vhd
// /home/antoine/Documents/Dynamatic/HistogramInt/hdl


// /home/antoine/Documents/Dynamatic/BigExamples/examples/matching/src/matching.cpp
// /home/antoine/Documents/Dynamatic/BigExamples/examples/matching/hdl
// matching_optimized.vhd

// /home/antoine/Documents/Dynamatic/TestBench/hdl
// histogram_elaborated_optimized.vhd
// /home/antoine/Documents/Dynamatic/TestBench_2/hdl

// /home/antoine/Documents/Dynamatic/BigExamples/examples/fir/hdl
// fir_elaborated_optimized.vhd
int main(void) {
    
    hdl_source_t hdl_source;
    CHECK_CALL(hdl_create(&hdl_source), "hdl_create failed !");
    CHECK_CALL(parse_hdl(&hdl_source), "parse_hdl failed !");

    vivado_hls_t hls;
    CHECK_CALL_DO(create_hls(&hls, &hdl_source), "create_hls failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(parse_hls(&hls, &hdl_source), "parse_hls failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(generate_hls_script(&hls), "generate_hls_script failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(launch_hls_script(), "launch_hls_script failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(find_float_op(&hls), "find_float_op failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(open_dot_file(&hls, &hdl_source), "open_dot_file failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(update_fop_tcl(&hls), "update_fop_tcl failed !", hdl_free(&hdl_source); hls_free(&hls););

    project_t project;
    CHECK_CALL_DO(create_project(&project, &hdl_source), "create_project failed !", hdl_free(&hdl_source); hls_free(&hls););

    axi_ip_t axi_ip;
    CHECK_CALL_DO(create_axi(&axi_ip, &project), "create_axi failed !", hdl_free(&hdl_source); hls_free(&hls););

    CHECK_CALL_DO(generate_AXI_script(&project, &axi_ip), "generate_AXI_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(generate_MAIN_script(&project), "generate_MAIN_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(launch_script("generate_project.tcl", hdl_source.exec_path), "launch_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    
    CHECK_CALL_DO(update_arithmetic_units(&project, &hls, &axi_ip), "update_arithmetic_units failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(read_axi_files(&axi_ip), "read_axi_files failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(update_files(&project, &axi_ip), "update_files failed !", project_free(&project); free_axi(&axi_ip););
    CHECK_CALL_DO(generate_final_script(&project, &hls, &axi_ip), "generate_final_script failed !", project_free(&project); hls_free(&hls); free_axi(&axi_ip););

    CHECK_CALL_DO(launch_script("final_script.tcl", hdl_source.exec_path), "launch_script failed !", project_free(&project); hls_free(&hls); free_axi(&axi_ip););

    CHECK_CALL_DO(project_free(&project), "project_free failed !", project_free(&project); hls_free(&hls); free_axi(&axi_ip););

    clean_folder();
    return 0;
}

#pragma clang diagnostic pop
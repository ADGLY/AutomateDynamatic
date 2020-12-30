#include <stdio.h>
#include <limits.h>
#include "file.h"
#include "hdl.h"
#include "axi_files.h"
#include "tcl.h"
#include "vivado.h"
#include "vivado_hls.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

//histogram_elaborated_optimized.vhd
///home/antoine/Documents/Dynamatic/HistogramInt/hdl


// /home/antoine/Documents/Dynamatic/BigExamples/examples/matching/src/matching.cpp
// /home/antoine/Documents/Dynamatic/BigExamples/examples/matching/hdl
// matching_optimized.vhd
int main(void) {
    
    hdl_source_t hdl_source;
    CHECK_CALL(hdl_create(&hdl_source), "hdl_create failed !");
    CHECK_CALL(parse_hdl(&hdl_source), "parse_hdl failed !");

    vivado_hls_t hls;
    CHECK_CALL_DO(create_hls(&hls, &hdl_source), "create_hls failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(parse_hls(&hls, &hdl_source), "parse_hls failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(generate_hls_script(&hls), "generate_hls_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(launch_hls_script(), "launch_hls_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    


    project_t project;
    CHECK_CALL_DO(create_project(&project, &hdl_source, &hls), "create_project failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(generate_AXI_script(&project), "generate_AXI_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(generate_MAIN_script(&project), "generate_MAIN_script failed !", hdl_free(&hdl_source); hls_free(&hls););

    CHECK_CALL_DO(launch_script("generate_project.tcl", hdl_source.exec_path), "launch_script failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(read_axi_files(&(project.axi_ip)), "read_axi_files failed !", hdl_free(&hdl_source); hls_free(&hls););
    CHECK_CALL_DO(update_files(&project), "update_files failed !", project_free(&project););

    CHECK_CALL_DO(generate_final_script(&project), "generate_final_script failed !", project_free(&project); hls_free(&hls););

    CHECK_CALL_DO(launch_script("final_script.tcl", hdl_source.exec_path), "launch_script failed !", project_free(&project); hls_free(&hls););

    CHECK_CALL_DO(project_free(&project), "project_free failed !", project_free(&project); hls_free(&hls););
    return 0;
}

#pragma clang diagnostic pop
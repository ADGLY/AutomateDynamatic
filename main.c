#include <stdio.h>
#include "file.h"
#include "hdl.h"
#include "axi_files.h"
#include "tcl.h"
#include "vivado.h"


//TODO: REfactor method to get path !

//histogram_elaborated_optimized.vhd
///home/antoine/Documents/Dynamatic/HistogramInt/hdl
int main(void) {
    hdl_source_t hdl_source;
    CHECK_CALL(hdl_create(&hdl_source), "hdl_create failed !");
    CHECK_CALL(parse_hdl(&hdl_source), "parse_hdl failed !");

    project_t project;
    CHECK_CALL_DO(create_project(&project, &hdl_source), "create_project failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(generate_AXI_script(&project), "generate_AXI_script failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(generate_MAIN_script(&project), "generate_MAIN_script failed !", hdl_free(&hdl_source););

    CHECK_CALL_DO(launch_script("generate_project.tcl", hdl_source.exec_path), "launch_script failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(read_axi_files(&(project.axi_ip)), "read_axi_files failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(update_files(&project), "update_files failed !", project_free(&project););


    CHECK_CALL_DO(generate_final_script(&project), "generate_final_script failed !", project_free(&project););

    CHECK_CALL_DO(launch_script("final_script.tcl", hdl_source.exec_path), "launch_script failed !", hdl_free(&hdl_source););

    CHECK_CALL_DO(project_free(&project), "project_free failed !", hdl_free(&hdl_source););
    return 0;
}
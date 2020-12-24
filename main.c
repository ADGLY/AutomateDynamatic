#include "file.h"
#include "hdl.h"
#include "axi_files.h"
#include "tcl.h"
#include "vivado.h"

//histogram_elaborated_optimized.vhd
///home/antoine/Documents/Dynamatic/HistogramInt/hdl
int main(void) {
    hdl_source_t hdl_source;
    hdl_create(&hdl_source);
    parse_hdl(&hdl_source);

    project_t project;
    create_project(&project, &hdl_source);
    generate_AXI_script(&project);
    generate_MAIN_script(&project);

    
    launch_script("generate_project.tcl");
    read_axi_files(&(project.axi_ip));
    update_files(&project);

    generate_final_script(&project);

    launch_script("final_script.tcl");

    project_free(&project);
    return 0;
}
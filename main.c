#include "file.h"
#include "hdl_data.h"
#include "axi_files.h"
#include "tcl.h"



//histogram_elaborated_optimized.vhd

int main(void) {
    hdl_source_t hdl_source;
    project_t project;
    hdl_create(&hdl_source);
    parse_hdl(&hdl_source);

    create_project(&project, &hdl_source);
    generate_AXI_script(&project);

    read_axi_files(&(project.axi_ip));
    update_files(&project);
    hdl_free(&hdl_source);
    return 0;
}
#include <stdio.h>
#include "file.h"
#include "hdl_data.h"
#include "axi_files.h"
#include "tcl.h"



//histogram_elaborated_optimized.vhd
///home/antoine/Documents/Dynamatic/HistogramInt/hdl

int main(void) {
    hdl_source_t hdl_source;
    project_t project;
    hdl_create(&hdl_source);
    parse_hdl(&hdl_source);

    create_project(&project, &hdl_source);
    generate_AXI_script(&project);
    generate_MAIN_script(&project);

    char buf[MAX_NAME_LENGTH];
    FILE* vivado = popen("vivado -mode tcl", "w");
    fputs("source /home/antoine/Documents/ProjetSemestre/Automation/AutomateDynamatic/generate_project.tcl\n", vivado);
    read_axi_files(&(project.axi_ip));
    update_files(&project);
    hdl_free(&hdl_source);
    return 0;
}
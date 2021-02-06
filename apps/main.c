#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "axi_files.h"
#include "hdl.h"
#include "tcl.h"
#include "vivado.h"
#include "vivado_hls.h"

// /home/antoine/Documents/Dynamatic/Examples/fir/hdl
// fir_optimized.vhd

// /home/antoine/Documents/Dynamatic/Examples/histogram/hdl
// histogram_optimized.vhd

// /home/antoine/Documents/Dynamatic/Examples/if_loop_add/hdl
// if_loop_add_optimized.vhd

// /home/antoine/Documents/Dynamatic/Examples/if_loop_mul/hdl
// if_loop_mul_optimized.vhd

// /home/antoine/Documents/Dynamatic/Examples/matching/hdl
// matching_optimized.vhd

// /home/antoine/Documents/Dynamatic/Examples/matrix_power/hdl
// matrix_power.vhd

// /home/antoine/Documents/Dynamatic/Examples/matvec/hdl
// matvec_optimized.vhd

// xc7z045ffg900-2 --> ZC706
// xc7z020clg484-1 --> ZC702
// xcvu9p-flgb2104-2-i --> AWS

int main(void) {
    printf("To use this tool, you will need to have at least analyzed the cpp "
           "source file using Dynamatic so that\n");
    printf("comments are removed.\n");

    char part_nb[MAX_NAME_LENGTH];
    memset(part_nb, 0, sizeof(char) * MAX_NAME_LENGTH);

    get_name(part_nb, "What is the target ? (part number)");

    hdl_source_t hdl_source;
    CHECK_CALL(hdl_create(&hdl_source), "hdl_create failed !");
    CHECK_CALL(parse_hdl(&hdl_source), "parse_hdl failed !");

    vivado_hls_t hls;
    CHECK_CALL_DO(create_hls(&hls, &hdl_source), "create_hls failed !",
                  hdl_free(&hdl_source););
    CHECK_CALL_DO(parse_hls(&hls, &hdl_source), "parse_hls failed !",
                  hdl_free(&hdl_source);
                  hls_free(&hls););

    CHECK_CALL_DO(generate_hls_script(&hls, part_nb),
                  "generate_hls_script failed !", hdl_free(&hdl_source););
    CHECK_CALL_DO(launch_hls_script(), "launch_hls_script failed !",
                  hdl_free(&hdl_source);
                  hls_free(&hls););

    CHECK_CALL_DO(resolve_float_ops(&hls, &hdl_source),
                  "resolve_float_ops failed !", hdl_free(&hdl_source);
                  hls_free(&hls););

    project_t project;
    CHECK_CALL_DO(create_project(&project, &hdl_source),
                  "create_project failed !", hdl_free(&hdl_source);
                  hls_free(&hls););

    axi_ip_t axi_ip;
    CHECK_CALL_DO(create_axi(&axi_ip, &project), "create_axi failed !",
                  free_axi(&axi_ip);
                  hls_free(&hls); project_free(&project););

    CHECK_CALL_DO(generate_AXI_script(&project, &axi_ip),
                  "generate_AXI_script failed !", free_axi(&axi_ip);
                  hls_free(&hls); project_free(&project););
    CHECK_CALL_DO(generate_MAIN_script(&project, part_nb),
                  "generate_MAIN_script failed !", free_axi(&axi_ip);
                  hls_free(&hls); project_free(&project););
    CHECK_CALL_DO(launch_script("generate_project.tcl", hdl_source.exec_path),
                  "launch_script failed !", free_axi(&axi_ip);
                  hls_free(&hls); project_free(&project););

    CHECK_CALL_DO(update_arithmetic_units(&project, &hls, &axi_ip),
                  "update_arithmetic_units failed !", hls_free(&hls);
                  project_free(&project); free_axi(&axi_ip););
    CHECK_CALL_DO(read_axi_files(&axi_ip), "read_axi_files failed !",
                  hls_free(&hls);
                  project_free(&project); free_axi(&axi_ip););
    CHECK_CALL_DO(update_files(&project, &axi_ip), "update_files failed !",
                  project_free(&project);
                  free_axi(&axi_ip); hls_free(&hls););
    CHECK_CALL_DO(generate_final_script(&project, &hls, &axi_ip, part_nb),
                  "generate_final_script failed !", project_free(&project);
                  hls_free(&hls); free_axi(&axi_ip););

    CHECK_CALL_DO(launch_script("final_script.tcl", hdl_source.exec_path),
                  "launch_script failed !", project_free(&project);
                  hls_free(&hls); free_axi(&axi_ip););

    CHECK_CALL_DO(project_free(&project), "project_free failed !",
                  hls_free(&hls);
                  free_axi(&axi_ip););
    CHECK_CALL_DO(hls_free(&hls), "hls_free failed !", free_axi(&axi_ip););
    CHECK_CALL(free_axi(&axi_ip), "free_axi failed !");

    clean_folder();
    return 0;
}
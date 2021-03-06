#include <stdio.h>
#include <string.h>
#include "regex_wrapper.h"
#include "axi_files.h"
#include "tcl.h"

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

    CHECK_CALL(get_name(part_nb, "What is the target ? (part number)"), "get_name failed !")

    hdl_info_t hdl_info;
    CHECK_CALL(hdl_create(&hdl_info), "hdl_create failed !")
    CHECK_CALL_DO(parse_hdl(&hdl_info), "parse_hdl failed !", free_regs();)

    vivado_hls_t hls;
    CHECK_CALL_DO(create_hls(&hls, &hdl_info), "create_hls failed !",
                  hdl_free(&hdl_info);
                          free_regs();)
    CHECK_CALL_DO(parse_hls(&hls, &hdl_info), "parse_hls failed !",
                  hdl_free(&hdl_info);
                          hls_free(&hls);
                          free_regs();)

    CHECK_CALL_DO(generate_hls_script(&hls, part_nb),
                  "generate_hls_script failed !", hdl_free(&hdl_info);
                          free_regs();)
    CHECK_CALL_DO(launch_hls_script(), "launch_hls_script failed !",
                  hdl_free(&hdl_info);
                          hls_free(&hls);
                          free_regs();)

    CHECK_CALL_DO(resolve_float_ops(&hls, &hdl_info),
                  "resolve_float_ops failed !", hdl_free(&hdl_info);
                          hls_free(&hls);
                          free_regs();)

    project_t project;
    CHECK_CALL_DO(create_project(&project, &hdl_info),
                  "create_project failed !", hdl_free(&hdl_info);
                          hls_free(&hls);
                          free_regs();)

    axi_ip_t axi_ip;
    CHECK_CALL_DO(create_axi(&axi_ip, &project), "create_axi failed !",
                  free_axi(&axi_ip);
                          hls_free(&hls); project_free(&project);
                          free_regs();)

    CHECK_CALL_DO(generate_AXI_script(&project, &axi_ip),
                  "generate_AXI_script failed !", free_axi(&axi_ip);
                          hls_free(&hls); project_free(&project);
                          free_regs();)
    CHECK_CALL_DO(generate_MAIN_script(&project, part_nb),
                  "generate_MAIN_script failed !", free_axi(&axi_ip);
                          hls_free(&hls); project_free(&project);
                          free_regs();)
    CHECK_CALL_DO(launch_script("generate_project.tcl", project.exec_path),
                  "launch_script failed !", free_axi(&axi_ip);
                          hls_free(&hls); project_free(&project);
                          free_regs();)

    CHECK_CALL_DO(update_arithmetic_units(&project, &hls, &axi_ip),
                  "update_arithmetic_units failed !", hls_free(&hls);
                          project_free(&project); free_axi(&axi_ip);
                          free_regs();)
    CHECK_CALL_DO(read_axi_files(&axi_ip), "read_axi_files failed !",
                  hls_free(&hls);
                          project_free(&project); free_axi(&axi_ip); free_regs();)
    CHECK_CALL_DO(update_files(&project, &axi_ip), "update_files failed !",
                  project_free(&project);
                          free_axi(&axi_ip); hls_free(&hls);
                          free_regs();)
    CHECK_CALL_DO(generate_final_script(&project, &hls, &axi_ip, part_nb),
                  "generate_final_script failed !", project_free(&project);
                          hls_free(&hls); free_axi(&axi_ip);
                          free_regs();)

    CHECK_CALL_DO(launch_script("final_script.tcl", project.exec_path),
                  "launch_script failed !", project_free(&project);
                          hls_free(&hls); free_axi(&axi_ip);
                          free_regs();)

    CHECK_CALL_DO(project_free(&project), "project_free failed !",
                  hls_free(&hls);
                          free_axi(&axi_ip);
                          free_regs();)
    CHECK_CALL_DO(hls_free(&hls), "hls_free failed !", free_axi(&axi_ip); free_regs();)
    CHECK_CALL_DO(free_axi(&axi_ip), "free_axi failed !", free_regs();)
    clean_folder();
    free_regs();
    return 0;
}
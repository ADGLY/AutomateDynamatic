#include <inttypes.h>
#include <string.h>
#include "tcl_board.h"


/*--------------------------------------------------------------------------------------------------------------
|                                     Zynq                                                                      |
|                                                                                                               |
|---------------------------------------------------------------------------------------------------------------|*/


void memory_connection_automation_zynq(FILE* tcl_script, hdl_source_t* hdl_source) {

    for(size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        hdl_array_t* arr = &(hdl_source->arrays[i]);
        const char* suffix;
        if(arr->write && arr->read) {
            suffix = "read_write";
        }
        else if(arr->write) {
            suffix = "write";
        }
        else if(arr->read) {
            suffix = "read";
        }
        fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config");
        fprintf(tcl_script, " { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0}");
        fprintf(tcl_script, " Slave {/axi_bram_ctrl_%s_%s/S_AXI} ddr_seg {Auto} intc_ip {New AXI Interconnect} master_apm {0}}", arr->name, suffix);
        fprintf(tcl_script, "  [get_bd_intf_pins axi_bram_ctrl_%s_%s/S_AXI]\n", arr->name, suffix);
    }
}

void zynq_create_PS(FILE* tcl_script, char* board) {
    fprintf(tcl_script, "set_property board_part xilinx.com:%s:part0:1.4 [current_project]\n", board);
    fprintf(tcl_script, "startgroup\n");
    str_toupper(board);
    fprintf(tcl_script, "set latest_ver [get_ipdefs -filter {NAME == processing_system7}]\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv $latest_ver processing_system7_0\n");
    fprintf(tcl_script, "endgroup\n");
    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "set_property -dict [list CONFIG.preset {%s}] [get_bd_cells processing_system7_0]\n", board);
    fprintf(tcl_script, "set_property -dict [list CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100}] [get_bd_cells processing_system7_0]\n");
    fprintf(tcl_script, "endgroup\n");
    fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:processing_system7");
    fprintf(tcl_script, " -config {make_external \"FIXED_IO, DDR\" apply_board_preset \"0\" Master \"Disable\" Slave \"Disable\" }  [get_bd_cells processing_system7_0]\n");
}

void zynq_connection_automation(FILE* tcl_script, project_t* project, axi_ip_t* axi_ip) {

    fprintf(tcl_script, "save_bd_design\n");
    fprintf(tcl_script, "startgroup\n");
    for(uint16_t i = 0; i < project->hdl_source->nb_arrays; ++i) {
        hdl_array_t* arr = &(project->hdl_source->arrays[i]);
        GET_SUFFIX(arr, suffix);

        fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:clkrst -config { Clk {/processing_system7_0/FCLK_CLK0 (100 MHz)} Freq {100} Ref_Clk0 {} Ref_Clk1 {} Ref_Clk2 {}}");
        fprintf(tcl_script, "  [get_bd_pins axi_bram_ctrl_%s_%s/s_axi_aclk]\n", arr->name, suffix);
    }

    fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:clkrst -config { Clk {/processing_system7_0/FCLK_CLK0 (100 MHz)} Freq {100} Ref_Clk0 {} Ref_Clk1 {} Ref_Clk2 {}}");
    fprintf(tcl_script, "  [get_bd_pins %s_0/csr_aclk]", axi_ip->name);

    hdl_array_t* first_arr = &(project->hdl_source->arrays[0]);
    GET_SUFFIX(first_arr, suffix)
    fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0}");
    fprintf(tcl_script, " Slave {/axi_bram_ctrl_%s_%s/S_AXI} ddr_seg {Auto} intc_ip {/smartconnect_0} master_apm {0}}", first_arr->name, suffix);
    fprintf(tcl_script, "  [get_bd_intf_pins processing_system7_0/M_AXI_GP0]\n");

    /*memory_connection_automation_zynq(tcl_script, project->hdl_source);
    fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0}");
    fprintf(tcl_script, " Slave {/%s_0/%s} ddr_seg {Auto} intc_ip {New AXI Interconnect} master_apm {0}}", axi_ip->name, axi_ip->interface_name);
    fprintf(tcl_script, "  [get_bd_intf_pins %s_0/%s]\n", axi_ip->name, axi_ip->interface_name);*/

    fprintf(tcl_script, "endgroup\n");

    for(size_t i = 0; i < project->hdl_source->nb_arrays; ++i) {
        hdl_array_t* arr = &(project->hdl_source->arrays[i]);
        GET_SUFFIX(arr, suffix_inter)
        fprintf(tcl_script, "connect_bd_net [get_bd_pins blk_mem_gen_%s_%s/clkb] [get_bd_pins processing_system7_0/FCLK_CLK0]\n", arr->name, suffix_inter);
    }

    for(size_t i = 0; i < project->hdl_source->nb_arrays; ++i) {
        hdl_array_t* arr = &(project->hdl_source->arrays[i]);
        GET_SUFFIX(arr, suffix_inter)
        fprintf(tcl_script, "set_property range " "%" PRIu16 "%c"  " [get_bd_addr_segs {processing_system7_0/Data/SEG_axi_bram_ctrl_%s_%s_Mem0}]\n",
            project->array_size, project->array_size_ind, arr->name, suffix_inter);
    }

}

void script_zc706(FILE* tcl_script, project_t* project, axi_ip_t* axi_ip) {

    char board[MAX_NAME_LENGTH];
    strncpy(board, "zc706", MAX_NAME_LENGTH);

    zynq_create_PS(tcl_script, board);
    zynq_connection_automation(tcl_script, project, axi_ip);
}

void script_zc702(FILE* tcl_script, project_t* project, axi_ip_t* axi_ip) {

    char board[MAX_NAME_LENGTH];
    strncpy(board, "zc702", MAX_NAME_LENGTH);

    zynq_create_PS(tcl_script, board);
    zynq_connection_automation(tcl_script, project, axi_ip);
}

script_func_t select_part_script(const char* part) {
    if(part == NULL) {
        return NULL;
    }
    //xc7z045ffg900-2 --> ZC706
    if(strcmp(part, "xc7z045ffg900-2") == 0) {
        return &script_zc706;
    }
    else if(strcmp(part, "xc7z020clg484-1") == 0) {
        return &script_zc702;
    }

    return NULL;
}

#include <stdio.h>
#include <regex.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "tcl.h"

error_t generate_MAIN_script(project_t* project) {
    CHECK_PARAM(project);

    FILE* tcl_script = fopen("generate_project.tcl", "w");
    CHECK_NULL(tcl_script, ERR_FILE, "Could not open file : generate_project.tcl");

    fprintf(tcl_script, "create_project %s %s/%s -part xc7z045ffg900-2\n", 
    project->name, project->path, project->name);
    fprintf(tcl_script, "set_property board_part xilinx.com:zc706:part0:1.4 [current_project]\n");
    fprintf(tcl_script, "set_property target_language VHDL [current_project]\n");
    char ip_script_path[MAX_NAME_LENGTH];
    
    strncpy(ip_script_path, project->hdl_source->exec_path, MAX_NAME_LENGTH);

    CHECK_COND_DO(strlen(ip_script_path) + strlen("/generate_axi_ip.tcl") + 1 >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG, "", fclose(tcl_script););
    
    strcpy(ip_script_path + strlen(ip_script_path), "/generate_axi_ip.tcl");

    fprintf(tcl_script, "source %s", ip_script_path);

    fclose(tcl_script);

    return ERR_NONE;

}

error_t generate_AXI_script(project_t* project) {
    CHECK_PARAM(project);

    axi_ip_t* axi_ip = &(project->axi_ip);
    strcpy(axi_ip->name, "axi_ip_dynamatic_test");
    strcpy(axi_ip->interface_name, "CSR");
    axi_ip->nb_slave_registers = project->hdl_source->nb_params + 2;

    //Minimum is four slave registers
    if(axi_ip->nb_slave_registers < 4) {
        axi_ip->nb_slave_registers = 4;
    }

    FILE* tcl_script = fopen("generate_axi_ip.tcl", "w");
    CHECK_NULL(tcl_script, ERR_FILE, "Could not open file : generate_axi_ip.tcl");

    fprintf(tcl_script, "create_peripheral user.org user %s 1.0 -dir %s\n", axi_ip->name,
    axi_ip->path);
    fprintf(tcl_script, "add_peripheral_interface %s -interface_mode slave -axi_type lite [ipx::find_open_core user.org:user:%s:1.0]\n",
    axi_ip->interface_name, axi_ip->name);
    fprintf(tcl_script, "set_property VALUE %zu [ipx::get_bus_parameters WIZ_NUM_REG -of_objects [ipx::get_bus_interfaces %s -of_objects [ipx::find_open_core user.org:user:%s:1.0]]]\n",
    axi_ip->nb_slave_registers, axi_ip->interface_name, axi_ip->name);
    fprintf(tcl_script, "generate_peripheral -bfm_example_design -debug_hw_example_design [ipx::find_open_core user.org:user:%s:1.0]\n", axi_ip->name);
    fprintf(tcl_script, "write_peripheral [ipx::find_open_core user.org:user:%s:1.0]\n", axi_ip->name);
    fprintf(tcl_script, "set_property  ip_repo_paths  %s/%s_1.0 [current_project]\n", axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "update_ip_catalog -rebuild\n");
    fprintf(tcl_script, "ipx::edit_ip_in_project -upgrade true -name edit_%s_v1_0 -directory %s %s/%s_1.0/component.xml\n",
    axi_ip->name, axi_ip->path, axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    regex_t reg;
    regmatch_t match[1];

    int err = regcomp(&reg, "\\w+.(vhd|v)", REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg compile error !", fclose(tcl_script););

    char files_to_add[8][MAX_NAME_LENGTH];
    DIR *d;
    struct dirent *dir;
    d = opendir(project->hdl_source->dir);
    int nb_files = 0;
    if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
            if(nb_files == 8) {
                fprintf(stderr, "Too many files !\n");
            }
            err = regexec(&reg, dir->d_name, 1, (regmatch_t*)match, 0);
            if(err == 0) {
                strcpy(files_to_add[nb_files++], dir->d_name);
            }
        }
        closedir(d);
    }
    regfree(&reg);
    fprintf(tcl_script, "add_files -norecurse -copy_to %s/%s_1.0/src {", axi_ip->path, axi_ip->name);

    for(int i = 0; i < nb_files; ++i) {
        if(i == nb_files - 1) {
            fprintf(tcl_script, "%s/%s", project->hdl_source->dir, files_to_add[i]);
        }
        else {
            fprintf(tcl_script, "%s/%s ", project->hdl_source->dir, files_to_add[i]);
        }
    }
    fprintf(tcl_script, "}\n");

    for(int i = 0; i < nb_files; ++i) {
        if(files_to_add[i][strlen(files_to_add[i]) - 1] == 'd') {
            fprintf(tcl_script, "set_property file_type {VHDL 2008} [get_files  ");
            fprintf(tcl_script, "%s/%s_1.0/src/%s]\n", axi_ip->path, axi_ip->name, files_to_add[i]);
        }
    }

    fclose(tcl_script);

    return ERR_NONE;
}

error_t generate_adapters() {
    FILE* write_enb_adapter = fopen("write_enb_adapter.vhd", "w");
    CHECK_NULL(write_enb_adapter, ERR_FILE, "Could not open file : write_enb_adapter.vhd");

    fprintf(write_enb_adapter, "library IEEE;\n");
    fprintf(write_enb_adapter, "use IEEE.STD_LOGIC_1164.ALL;\n\n");

    fprintf(write_enb_adapter, "entity write_enb_adapter is\n");
    fprintf(write_enb_adapter, "\tport ( axi_write_enb : in std_logic_vector(3 downto 0);\n");
    fprintf(write_enb_adapter, "\t   bram_write_enb : out std_logic);\n");
    fprintf(write_enb_adapter, "end write_enb_adapter;\n\n");
    fprintf(write_enb_adapter, "architecture Behavioral of write_enb_adapter is\n");
    fprintf(write_enb_adapter, "begin\n");
    fprintf(write_enb_adapter, "\tbram_write_enb <= '1' when axi_write_enb=\"1111\" else '0';\n");
    fprintf(write_enb_adapter, "end Behavioral;\n");

    fclose(write_enb_adapter);

    FILE* address_adapter = fopen("address_adapter.vhd", "w");
    CHECK_NULL(address_adapter, ERR_FILE, "Could not open file : address_adapter.vhd");

    fprintf(address_adapter, "library IEEE;\n");
    fprintf(address_adapter, "use IEEE.STD_LOGIC_1164.ALL;\n\n");

    fprintf(address_adapter, "entity address_adapter is\n");
    fprintf(address_adapter, "\tport ( axi_addr : in std_logic_vector(12 downto 0);\n");
    fprintf(address_adapter, "\t   bram_addr : out std_logic_vector(10 downto 0));\n");
    fprintf(address_adapter, "end address_adapter;\n\n");
    fprintf(address_adapter, "architecture Behavioral of address_adapter is\n");
    fprintf(address_adapter, "begin\n");
    fprintf(address_adapter, "\tbram_addr <= axi_addr(12 downto 2);\n");
    fprintf(address_adapter, "end Behavioral;\n");

    fclose(address_adapter);

    return ERR_NONE;
}


error_t generate_memory_interface(FILE* tcl_script, axi_ip_t* axi_ip, const char* name, const char* suffix, bram_interface_t* interface) {
    CHECK_PARAM(tcl_script);
    CHECK_PARAM(axi_ip);
    CHECK_PARAM(name);
    CHECK_PARAM(suffix);
    CHECK_PARAM(interface);

    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 blk_mem_gen_%s_%s\n", name, suffix);
    fprintf(tcl_script, "endgroup\n");


    fprintf(tcl_script, "set_property -dict [list CONFIG.Memory_Type {True_Dual_Port_RAM} ");
    fprintf(tcl_script, "CONFIG.Enable_32bit_Address {false} CONFIG.Use_Byte_Write_Enable {false} ");
    fprintf(tcl_script, "CONFIG.Byte_Size {9} CONFIG.Assume_Synchronous_Clk {true} CONFIG.Write_Depth_A {2048} ");
    fprintf(tcl_script, "CONFIG.Enable_B {Use_ENB_Pin} CONFIG.Register_PortA_Output_of_Memory_Primitives {false} ");
    fprintf(tcl_script, "CONFIG.Register_PortB_Output_of_Memory_Primitives {false} CONFIG.Use_RSTA_Pin {false} ");
    fprintf(tcl_script, "CONFIG.Port_B_Clock {100} CONFIG.Port_B_Write_Rate {50} CONFIG.Port_B_Enable_Rate {100} ");
    fprintf(tcl_script, "CONFIG.use_bram_block {Stand_Alone} CONFIG.EN_SAFETY_CKT {false}] [get_bd_cells blk_mem_gen_%s_%s]\n", name, suffix);
    
    
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_%s/addrb]\n", axi_ip->name, interface->address, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_%s/enb]\n", axi_ip->name, interface->ce, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_%s/web]\n", axi_ip->name, interface->we, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_%s/doutb]\n", axi_ip->name, interface->din, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_%s/dinb]\n", axi_ip->name, interface->dout, name, suffix);

    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_%s_%s\n", name, suffix);
    fprintf(tcl_script, "endgroup\n");

    fprintf(tcl_script, "set_property -dict [list CONFIG.PROTOCOL {AXI4LITE} CONFIG.SINGLE_PORT_BRAM {1} CONFIG.ECC_TYPE {0}] [get_bd_cells axi_bram_ctrl_%s_%s]\n", name, suffix);
    fprintf(tcl_script, "create_bd_cell -type module -reference write_enb_adapter write_enb_adapter_%s_%s\n", name, suffix);
    fprintf(tcl_script, "create_bd_cell -type module -reference address_adapter address_adapter_%s_%s\n", name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_addr_a] [get_bd_pins address_adapter_%s_%s/axi_addr]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins address_adapter_%s_%s/bram_addr] [get_bd_pins blk_mem_gen_%s_%s/addra]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_wrdata_a] [get_bd_pins blk_mem_gen_%s_%s/dina]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_rddata_a] [get_bd_pins blk_mem_gen_%s_%s/douta]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_en_a] [get_bd_pins blk_mem_gen_%s_%s/ena]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_we_a] [get_bd_pins write_enb_adapter_%s_%s/axi_write_enb]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins write_enb_adapter_%s_%s/bram_write_enb] [get_bd_pins blk_mem_gen_%s_%s/wea]\n", name, suffix, name, suffix);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_clk_a] [get_bd_pins blk_mem_gen_%s_%s/clka]\n", name, suffix, name, suffix);

    return ERR_NONE;
}

error_t generate_memory(FILE* tcl_script, hdl_array_t* arr, axi_ip_t* axi_ip) {
    CHECK_PARAM(tcl_script);
    CHECK_PARAM(arr);
    CHECK_PARAM(axi_ip);

    CHECK_CALL(generate_memory_interface(tcl_script, axi_ip, arr->name, "write", &(arr->write_ports)), "generate_memory_interface failed !");
    CHECK_CALL(generate_memory_interface(tcl_script, axi_ip, arr->name, "read", &(arr->read_ports)), "generate_memory_interface failed !");

    return ERR_NONE;
}

error_t memory_connection_automation(FILE* tcl_script, hdl_source_t* hdl_source) {
    CHECK_PARAM(tcl_script);
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->arrays);

    for(size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        hdl_array_t* arr = &(hdl_source->arrays[i]);

        //Write
        fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config");
        fprintf(tcl_script, " { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0}");
        fprintf(tcl_script, " Slave {/axi_bram_ctrl_%s_write/S_AXI} ddr_seg {Auto} intc_ip {New AXI Interconnect} master_apm {0}}", arr->name);
        fprintf(tcl_script, "  [get_bd_intf_pins axi_bram_ctrl_%s_write/S_AXI]\n", arr->name);
        
        //Read
        fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config");
        fprintf(tcl_script, " { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0}");
        fprintf(tcl_script, " Slave {/axi_bram_ctrl_%s_read/S_AXI} ddr_seg {Auto} intc_ip {New AXI Interconnect} master_apm {0}}", arr->name);
        fprintf(tcl_script, "  [get_bd_intf_pins axi_bram_ctrl_%s_read/S_AXI]\n", arr->name);
    }

    return ERR_NONE;
    
}

error_t generate_final_script(project_t* project) {
    CHECK_PARAM(project);
    CHECK_PARAM(project->hdl_source);
    CHECK_PARAM(project->hdl_source->arrays);
    CHECK_PARAM(project->hdl_source->params);

    axi_ip_t* axi_ip = &(project->axi_ip);
    FILE* tcl_script = fopen("final_script.tcl", "w");
    CHECK_NULL(tcl_script, ERR_FILE, "Could not open file : final_script.tcl");

    //open_project /home/antoine/Documents/ProjetSemestre/Automation/AutomateDynamatic/Vivado_final_test/ip_repo/edit_axi_ip_dynamatic_test_v1_0.xpr
    //Double the open project ?
    fprintf(tcl_script, "open_project %s/edit_%s_v1_0.xpr\n", axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    //ipx::open_ipxact_file /home/antoine/Documents/ProjetSemestre/Automation/AutomateDynamatic/Vivado_final/ip_repo/axi_ip_dynamatic_test_1.0/component.xml

    fprintf(tcl_script, "ipx::open_ipxact_file %s/%s_1.0/component.xml\n", axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "ipx::merge_project_changes hdl_parameters [ipx::current_core]\n");
    axi_ip->revision = axi_ip->revision + 1;
    fprintf(tcl_script, "set_property core_revision %d [ipx::current_core]\n", axi_ip->revision);
    fprintf(tcl_script, "ipx::create_xgui_files [ipx::current_core]\n");
    fprintf(tcl_script, "ipx::update_checksums [ipx::current_core]\n");
    fprintf(tcl_script, "ipx::save_core [ipx::current_core]\n");

    //update_ip_catalog -rebuild -repo_path /home/antoine/Documents/ProjetSemestre/Automation/AutomateDynamatic/Vivado_final/ip_repo/axi_ip_dynamatic_test_1.0
    fprintf(tcl_script, "update_ip_catalog -rebuild -repo_path %s/%s_1.0\n", axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "close_project\n");

    //open_project /home/antoine/Documents/ProjetSemestre/Automation/AutomateDynamatic/Vivado_final/FinalProject/FinalProject.xpr

    fprintf(tcl_script, "open_project %s/%s/%s.xpr\n", project->path, project->name, project->name);
    fprintf(tcl_script, "update_ip_catalog -rebuild\n");

    fprintf(tcl_script, "create_bd_design \"design_1\"\n");
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv user.org:user:%s:1.0 %s_0\n", axi_ip->name, axi_ip->name);
    fprintf(tcl_script, "endgroup\n");

    CHECK_CALL_DO(generate_adapters(), "generate_adapters failed !", fclose(tcl_script););

    char adapters[MAX_NAME_LENGTH];
    strncpy(adapters, project->hdl_source->exec_path, MAX_NAME_LENGTH);

    fprintf(tcl_script, "import_files -norecurse {%s/write_enb_adapter.vhd %s/address_adapter.vhd}\n", adapters, adapters);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");


    for(size_t i = 0; i < project->hdl_source->nb_arrays; ++i) {
        CHECK_CALL_DO(generate_memory(tcl_script, &(project->hdl_source->arrays[i]), axi_ip), "generate_memory failed !", fclose(tcl_script););
    }


    //Specific to Zynq
    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0\n");
    fprintf(tcl_script, "endgroup\n");
    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "set_property -dict [list CONFIG.preset {ZC706}] [get_bd_cells processing_system7_0]\n");
    fprintf(tcl_script, "set_property -dict [list CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100}] [get_bd_cells processing_system7_0]\n");
    fprintf(tcl_script, "endgroup\n");
    fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:processing_system7");
    //No apply board preset ?
    fprintf(tcl_script, " -config {make_external \"FIXED_IO, DDR\" apply_board_preset \"0\" Master \"Disable\" Slave \"Disable\" }  [get_bd_cells processing_system7_0]\n");
    
    fprintf(tcl_script, "startgroup\n");
    CHECK_CALL_DO(memory_connection_automation(tcl_script, project->hdl_source), "memory_connection_automation failed !", fclose(tcl_script););
    fprintf(tcl_script, "apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0}");
    fprintf(tcl_script, " Slave {/%s_0/%s} ddr_seg {Auto} intc_ip {New AXI Interconnect} master_apm {0}}", axi_ip->name, axi_ip->interface_name);
    fprintf(tcl_script, "  [get_bd_intf_pins %s_0/%s]\n", axi_ip->name, axi_ip->interface_name);
    fprintf(tcl_script, "endgroup\n");

    for(size_t i = 0; i < project->hdl_source->nb_arrays; ++i) {
        hdl_array_t* arr = &(project->hdl_source->arrays[i]);
        fprintf(tcl_script, "connect_bd_net [get_bd_pins blk_mem_gen_%s_write/clkb] [get_bd_pins processing_system7_0/FCLK_CLK0]\n", arr->name);
        fprintf(tcl_script, "connect_bd_net [get_bd_pins blk_mem_gen_%s_read/clkb] [get_bd_pins processing_system7_0/FCLK_CLK0]\n", arr->name);
    }


    fprintf(tcl_script, "save_bd_design\n");

    fclose(tcl_script);

    return ERR_NONE;
}


/*//For each array
    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 blk_mem_gen_%s_write\n", arr->name);
    fprintf(tcl_script, "endgroup\n");


    fprintf(tcl_script, "set_property -dict [list CONFIG.Memory_Type {True_Dual_Port_RAM} ");
    fprintf(tcl_script, "CONFIG.Enable_32bit_Address {false} CONFIG.Use_Byte_Write_Enable {false} ");
    fprintf(tcl_script, "CONFIG.Byte_Size {9} CONFIG.Assume_Synchronous_Clk {true} CONFIG.Write_Depth_A {2048} ");
    fprintf(tcl_script, "CONFIG.Enable_B {Use_ENB_Pin} CONFIG.Register_PortA_Output_of_Memory_Primitives {false} ");
    fprintf(tcl_script, "CONFIG.Register_PortB_Output_of_Memory_Primitives {false} CONFIG.Use_RSTA_Pin {false} ");
    fprintf(tcl_script, "CONFIG.Port_B_Clock {100} CONFIG.Port_B_Write_Rate {50} CONFIG.Port_B_Enable_Rate {100} ");
    fprintf(tcl_script, "CONFIG.use_bram_block {Stand_Alone} CONFIG.EN_SAFETY_CKT {false}] [get_bd_cells blk_mem_gen_%s_write]\n", arr->name);
    
    
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_write/addrb]\n", axi_ip->name, arr->write_ports.address, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_write/enb]\n", axi_ip->name, arr->write_ports.ce, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_write/web]\n", axi_ip->name, arr->write_ports.we, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_write/dinb]\n", axi_ip->name, arr->write_ports.din, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins blk_mem_gen_%s_write/doutb]\n", axi_ip->name, arr->write_ports.dout, arr->name);

    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script, "create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_%s_write\n", arr->name);
    fprintf(tcl_script, "endgroup\n");

    fprintf(tcl_script, "set_property -dict [list CONFIG.PROTOCOL {AXI4LITE} CONFIG.SINGLE_PORT_BRAM {1} CONFIG.ECC_TYPE {0}] [get_bd_cells axi_bram_ctrl_%s_write]\n", arr->name);
    fprintf(tcl_script, "create_bd_cell -type module -reference write_enb_adapter write_enb_adapter_%s_write\n", arr->name);
    fprintf(tcl_script, "create_bd_cell -type module -reference address_adapter address_adapter_%s_write\n", arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_write/bram_addr_a] [get_bd_pins address_adapter_%s_write/axi_addr]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins address_adapter_%s_write/bram_addr] [get_bd_pins blk_mem_gen_%s_write/addra]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_write/bram_wrdata_a] [get_bd_pins blk_mem_gen_%s_write/dina]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_write/bram_rddata_a] [get_bd_pins blk_mem_gen_%s_write/douta]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_write/bram_en_a] [get_bd_pins blk_mem_gen_%s_write/ena]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_write/bram_we_a] [get_bd_pins write_enb_adapter_%s_write/axi_write_enb]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins write_enb_adapter_%s_write/bram_write_enb] [get_bd_pins blk_mem_gen_%s_write/wea]\n", arr->name, arr->name);
    fprintf(tcl_script, "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_write/bram_clk_a] [get_bd_pins blk_mem_gen_%s_write/clka]\n", arr->name, arr->name);
    */
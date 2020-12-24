
#include <stdio.h>
#include <regex.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "tcl.h"

void generate_MAIN_script(project_t* project) {
    FILE* tcl_script = fopen("generate_project.tcl", "w");
    fprintf(tcl_script, "create_project %s %s/%s -part xc7z045ffg900-2\n", 
    project->name, project->path, project->name);
    fprintf(tcl_script, "set_property board_part xilinx.com:zc706:part0:1.4 [current_project]\n");
    fprintf(tcl_script, "set_property target_language VHDL [current_project]\n");
    char ip_script_path[MAX_NAME_LENGTH];
    char* result = getcwd(ip_script_path, MAX_NAME_LENGTH);
    if(result == NULL) {
        fprintf(stderr, "getcwd error !\n");
    }
    if(strlen(ip_script_path) + strlen("/generate_axi_ip.tcl") + 1 >= MAX_NAME_LENGTH) {
        fprintf(stderr, "The path is too long !\n");
    }
    strcpy(ip_script_path + strlen(ip_script_path), "/generate_axi_ip.tcl");

    fprintf(tcl_script, "source %s", ip_script_path);

    fclose(tcl_script);


}

void generate_AXI_script(project_t* project) {
    axi_ip_t* axi_ip = &(project->axi_ip);
    strcpy(axi_ip->name, "axi_ip_dynamatic_test");
    strcpy(axi_ip->interface_name, "CSR");
    axi_ip->nb_slave_registers = project->hdl_source->nb_params + 2;

    //Minimum is four slave registers
    if(axi_ip->nb_slave_registers < 4) {
        axi_ip->nb_slave_registers = 4;
    }

    FILE* tcl_script = fopen("generate_axi_ip.tcl", "w");
    if(tcl_script == NULL) {
        fclose(tcl_script);
        fprintf(stderr, "The program did not manage to create the script !\n");
        return;
    }
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
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    

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
}

void generate_adapters() {
    FILE* write_enb_adapter = fopen("write_enb_adapter.vhd", "w");
    if(write_enb_adapter == NULL) {
        fprintf(stderr, "Could not open write adapter file !\n");
        return;
    }

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
    if(address_adapter == NULL) {
        fprintf(stderr, "Could not open address adapter file !\n");
        return;
    }

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
}


void generate_memory_interface(FILE* tcl_script, axi_ip_t* axi_ip, const char* name, const char* suffix, bram_interface_t* interface) {
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


}

void generate_memory(FILE* tcl_script, hdl_array_t* arr, axi_ip_t* axi_ip) {
    generate_memory_interface(tcl_script, axi_ip, arr->name, "write", &(arr->write_ports));
    generate_memory_interface(tcl_script, axi_ip, arr->name, "read", &(arr->read_ports));
}


void generate_final_script(project_t* project) {
    axi_ip_t* axi_ip = &(project->axi_ip);
    FILE* tcl_script = fopen("final_script.tcl", "w");
    if(tcl_script == NULL) {
        fclose(tcl_script);
        fprintf(stderr, "The program did not manage to create the script !\n");
        return;
    }
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

    generate_adapters();

    char adapters[MAX_NAME_LENGTH];
    char* result = getcwd(adapters, MAX_NAME_LENGTH);
    if(result == NULL) {
        fprintf(stderr, "getcwd error !\n");
        return;
    }

    fprintf(tcl_script, "import_files -norecurse {%s/write_enb_adapter.vhd %s/address_adapter.vhd}\n", adapters, adapters);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");


    for(size_t i = 0; i < project->hdl_source->nb_arrays; ++i) {
        generate_memory(tcl_script, &(project->hdl_source->arrays[i]), axi_ip);
    }

    fprintf(tcl_script, "save_bd_design\n");

    fclose(tcl_script);
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
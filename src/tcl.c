
#include "tcl.h"
#include "tcl_board.h"
#include "regex_wrapper.h"
#include <dirent.h>
#include <inttypes.h>
#include <string.h>

#define MAX_DYNAMATIC_FILES 8

auto_error_t generate_MAIN_script(project_t *project, const char *part) {
    CHECK_PARAM(project)
    CHECK_PARAM(part)

    FILE *tcl_script = fopen("generate_project.tcl", "w");
    CHECK_NULL(tcl_script, ERR_IO,
               "Could not open file : generate_project.tcl")
    // xc7z045ffg900-2
    if (strcmp(part, "xcvu9p-flgb2104-2-i") == 0) {
        fprintf(tcl_script, "create_project %s %s/%s -part xc7z045ffg900-2\n",
                project->name, project->path, project->name);
    } else {
        fprintf(tcl_script, "create_project %s %s/%s -part %s\n", project->name,
                project->path, project->name, part);
    }
    fprintf(tcl_script,
            "set_property target_language VHDL [current_project]\n");
    char ip_script_path[MAX_PATH_LENGTH];
    memset(ip_script_path, 0, MAX_PATH_LENGTH * sizeof(char));

    strncpy(ip_script_path, project->exec_path, MAX_PATH_LENGTH);

    CHECK_COND_DO(strlen(ip_script_path) + strlen("/generate_axi_ip.tcl") + 1 >=
                  MAX_PATH_LENGTH,
                  ERR_NAME_TOO_LONG, "", fclose(tcl_script);)

    strncpy(ip_script_path + strlen(ip_script_path), "/generate_axi_ip.tcl",
            MAX_PATH_LENGTH - strlen(ip_script_path));

    fprintf(tcl_script, "source %s", ip_script_path);

    fclose(tcl_script);

    return ERR_NONE;
}

auto_error_t generate_AXI_script(project_t *project, axi_ip_t *axi_ip) {
    CHECK_PARAM(project)
    CHECK_PARAM(axi_ip)
    CHECK_PARAM(project->hdl_info)

    strncpy(axi_ip->interface_name, "CSR", MAX_NAME_LENGTH);

    FILE *tcl_script = fopen("generate_axi_ip.tcl", "w");
    CHECK_NULL(tcl_script, ERR_IO, "Could not open file : generate_axi_ip.tcl")

    fprintf(tcl_script, "create_peripheral user.org user %s 1.0 -dir %s\n",
            axi_ip->name, axi_ip->path);
    fprintf(tcl_script,
            "add_peripheral_interface %s -interface_mode slave -axi_type lite "
            "[ipx::find_open_core user.org:user:%s:1.0]\n",
            axi_ip->interface_name, axi_ip->name);
    fprintf(tcl_script,
            "set_property VALUE %zu [ipx::get_bus_parameters WIZ_NUM_REG "
            "-of_objects [ipx::get_bus_interfaces %s -of_objects "
            "[ipx::find_open_core user.org:user:%s:1.0]]]\n",
            axi_ip->nb_slave_registers, axi_ip->interface_name, axi_ip->name);
    fprintf(tcl_script,
            "generate_peripheral -bfm_example_design -debug_hw_example_design "
            "[ipx::find_open_core user.org:user:%s:1.0]\n",
            axi_ip->name);
    fprintf(tcl_script,
            "write_peripheral [ipx::find_open_core user.org:user:%s:1.0]\n",
            axi_ip->name);
    fprintf(tcl_script,
            "set_property  ip_repo_paths  %s/%s_1.0 [current_project]\n",
            axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "update_ip_catalog -rebuild\n");
    fprintf(tcl_script,
            "ipx::edit_ip_in_project -upgrade true -name edit_%s_v1_0 "
            "-directory %s %s/%s_1.0/component.xml\n",
            axi_ip->name, axi_ip->path, axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    regmatch_t match[1];


    CHECK_CALL(set_pattern("\\w+.(vhd|v)", 1), "set_pattern failed !")

    char files_to_add[MAX_DYNAMATIC_FILES][MAX_NAME_LENGTH];
    memset(files_to_add, 0, sizeof(files_to_add));

    DIR *d;
    d = opendir(project->hdl_info->dir);
    CHECK_COND_DO(d == NULL, ERR_IO, "Could not open dir !",
                  fclose(tcl_script);)

    int nb_files = 0;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (nb_files == MAX_DYNAMATIC_FILES) {
            fprintf(stderr, "Too many files !\n");
        }
        auto_error_t err = find_set_pattern(dir->d_name, 1, match, 1);
        if (err == ERR_NONE) {
            CHECK_LENGTH(strlen(dir->d_name), MAX_NAME_LENGTH)
            strncpy(files_to_add[nb_files++], dir->d_name, MAX_NAME_LENGTH);
        }
    }
    closedir(d);
    fprintf(tcl_script, "add_files -norecurse -copy_to %s/%s_1.0/src {",
            axi_ip->path, axi_ip->name);

    for (int i = 0; i < nb_files; ++i) {
        if (i == nb_files - 1) {
            fprintf(tcl_script, "%s/%s", project->hdl_info->dir,
                    files_to_add[i]);
        } else {
            fprintf(tcl_script, "%s/%s ", project->hdl_info->dir,
                    files_to_add[i]);
        }
    }
    fprintf(tcl_script, "}\n");

    for (int i = 0; i < nb_files; ++i) {
        if (files_to_add[i][strlen(files_to_add[i]) - 1] == 'd') {
            fprintf(tcl_script,
                    "set_property file_type {VHDL 2008} [get_files  ");
            fprintf(tcl_script, "%s/%s_1.0/src/%s]\n", axi_ip->path,
                    axi_ip->name, files_to_add[i]);
        }
    }

    fclose(tcl_script);

    return ERR_NONE;
}

auto_error_t generate_adapters(FILE *tcl_script, const char *exec_path,
                               hdl_array_t *array, const char *suffix,
                               size_t array_size) {

    char write_enb_filename[MAX_NAME_LENGTH];
    memset(write_enb_filename, 0, sizeof(char) * MAX_NAME_LENGTH);
    const char *hdl_ext = ".vhd";
    int written = snprintf(write_enb_filename, MAX_NAME_LENGTH,
                           "write_enb_adapter%s%s", suffix, hdl_ext);
    CHECK_LENGTH(written, MAX_NAME_LENGTH)

    FILE *write_enb_adapter = fopen(write_enb_filename, "w");
    CHECK_NULL(write_enb_adapter, ERR_IO,
               "Could not open file : write_enb_adapter.vhd")

    fprintf(write_enb_adapter, "library IEEE;\n");
    fprintf(write_enb_adapter, "use IEEE.STD_LOGIC_1164.ALL;\n\n");

    uint16_t nb_bytes_in_width = (uint16_t) (array->width >> 3);
    int msb_axi = 64 - __builtin_clzll(array_size) - 2;

    size_t nb_elem = array_size / (array->width >> 3);
    int msb_bram = 64 - __builtin_clzll(nb_elem) - 2;

    fprintf(write_enb_adapter, "entity write_enb_adapter%s is\n", suffix);
    fprintf(write_enb_adapter,
            "\tport ( axi_write_enb : in std_logic_vector(%" PRIu16
            " downto 0);\n",
            nb_bytes_in_width - 1);
    fprintf(write_enb_adapter, "\t   bram_write_enb : out std_logic);\n");
    fprintf(write_enb_adapter, "end write_enb_adapter%s;\n\n", suffix);
    fprintf(write_enb_adapter,
            "architecture Behavioral of write_enb_adapter%s is\n", suffix);
    fprintf(write_enb_adapter, "begin\n");

#define MAX_NB_ONES 64
    char nb_ones[MAX_NB_ONES + 1];
    memset(nb_ones, 0, sizeof(char) * (MAX_NB_ONES + 1));

    for (uint16_t i = 0; i < nb_bytes_in_width; ++i) {
        nb_ones[i] = '1';
    }

    fprintf(write_enb_adapter,
            "\tbram_write_enb <= '1' when axi_write_enb=\"%s\" else '0';\n",
            nb_ones);
    fprintf(write_enb_adapter, "end Behavioral;\n");

    fclose(write_enb_adapter);

    char address_adapter_filename[MAX_NAME_LENGTH];
    memset(address_adapter_filename, 0, sizeof(char) * MAX_NAME_LENGTH);
    written = snprintf(address_adapter_filename, MAX_NAME_LENGTH,
                       "address_adapter%s%s", suffix, hdl_ext);
    CHECK_LENGTH(written, MAX_NAME_LENGTH)
    FILE *address_adapter = fopen(address_adapter_filename, "w");
    CHECK_NULL(address_adapter, ERR_IO,
               "Could not open file : address_adapter.vhd")

    fprintf(address_adapter, "library IEEE;\n");
    fprintf(address_adapter, "use IEEE.STD_LOGIC_1164.ALL;\n\n");

    int offset_msb = 64 - __builtin_clzll(nb_bytes_in_width) - 1;

    fprintf(address_adapter, "entity address_adapter%s is\n", suffix);
    fprintf(address_adapter,
            "\tport ( axi_addr : in std_logic_vector(%d downto 0);\n", msb_axi);
    fprintf(address_adapter,
            "\t   bram_addr : out std_logic_vector(%d downto 0));\n", msb_bram);
    fprintf(address_adapter, "end address_adapter%s;\n\n", suffix);
    fprintf(address_adapter,
            "architecture Behavioral of address_adapter%s is\n", suffix);
    fprintf(address_adapter, "begin\n");
    fprintf(address_adapter, "\tbram_addr <= axi_addr(%d downto %d);\n",
            msb_axi, offset_msb);
    fprintf(address_adapter, "end Behavioral;\n");

    fclose(address_adapter);

    fprintf(tcl_script,
            "import_files -norecurse {%s/write_enb_adapter%s.vhd "
            "%s/address_adapter%s.vhd}\n",
            exec_path, suffix, exec_path, suffix);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    return ERR_NONE;
}

void create_bram(FILE *tcl_script, const char *name, const char *suffix,
                 size_t array_size, size_t array_width) {
    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script,
            "set latest_ver [get_ipdefs -filter {NAME == blk_mem_gen}]\n");
    fprintf(tcl_script,
            "create_bd_cell -type ip -vlnv $latest_ver blk_mem_gen_%s_%s\n",
            name, suffix);
    fprintf(tcl_script, "endgroup\n");

    fprintf(
            tcl_script,
            "set_property -dict [list CONFIG.Memory_Type {True_Dual_Port_RAM} ");
    fprintf(tcl_script, "CONFIG.Enable_32bit_Address {false} "
                        "CONFIG.Use_Byte_Write_Enable {false} ");
    fprintf(tcl_script,
            "CONFIG.Byte_Size {9} CONFIG.Assume_Synchronous_Clk {true} "
            "CONFIG.Write_Width_A {%zu} ",
            array_width);
    fprintf(tcl_script, "CONFIG.Write_Depth_A {%zu} CONFIG.Read_Width_A {%zu} ",
            array_size / (array_width >> 3), array_width);
    fprintf(tcl_script, "CONFIG.Write_Width_B {%zu} CONFIG.Read_Width_B {%zu} ",
            array_width, array_width);
    fprintf(tcl_script,
            "CONFIG.Enable_B {Use_ENB_Pin} "
            "CONFIG.Register_PortA_Output_of_Memory_Primitives {false} ");
    fprintf(tcl_script, "CONFIG.Register_PortB_Output_of_Memory_Primitives "
                        "{false} CONFIG.Use_RSTA_Pin {false} ");
    fprintf(tcl_script, "CONFIG.Port_B_Clock {100} CONFIG.Port_B_Write_Rate "
                        "{50} CONFIG.Port_B_Enable_Rate {100} ");
    fprintf(tcl_script,
            "CONFIG.use_bram_block {Stand_Alone} CONFIG.EN_SAFETY_CKT {false}] "
            "[get_bd_cells blk_mem_gen_%s_%s]\n",
            name, suffix);
}

void create_axi_bram_controller(FILE *tcl_script, const char *name,
                                const char *suffix, size_t array_width) {
    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script,
            "set latest_ver [get_ipdefs -filter {NAME == axi_bram_ctrl}]\n");
    fprintf(tcl_script,
            "create_bd_cell -type ip -vlnv $latest_ver axi_bram_ctrl_%s_%s\n",
            name, suffix);
    fprintf(tcl_script, "endgroup\n");

    fprintf(
            tcl_script,
            "set_property -dict [list CONFIG.SINGLE_PORT_BRAM {1} CONFIG.ECC_TYPE "
            "{0}  CONFIG.DATA_WIDTH {%zu} CONFIG.PROTOCOL {AXI4}] ",
            array_width);
    fprintf(tcl_script, "[get_bd_cells axi_bram_ctrl_%s_%s]\n", name, suffix);
}

auto_error_t generate_memory_interface(FILE *tcl_script, axi_ip_t *axi_ip,
                                       const char *name, const char *suffix,
                                       bram_interface_t *interface,
                                       size_t array_size,
                                       const char *adapter_suffix,
                                       size_t array_width) {
    CHECK_PARAM(tcl_script)
    CHECK_PARAM(axi_ip)
    CHECK_PARAM(name)
    CHECK_PARAM(suffix)
    CHECK_PARAM(interface)

    create_bram(tcl_script, name, suffix, array_size, array_width);

    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
            "blk_mem_gen_%s_%s/addrb]\n",
            axi_ip->name, interface->address, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
            "blk_mem_gen_%s_%s/enb]\n",
            axi_ip->name, interface->ce, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
            "blk_mem_gen_%s_%s/web]\n",
            axi_ip->name, interface->we, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
            "blk_mem_gen_%s_%s/doutb]\n",
            axi_ip->name, interface->din, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
            "blk_mem_gen_%s_%s/dinb]\n",
            axi_ip->name, interface->dout, name, suffix);

    create_axi_bram_controller(tcl_script, name, suffix, array_width);

    fprintf(tcl_script,
            "create_bd_cell -type module -reference write_enb_adapter%s "
            "write_enb_adapter%s\n",
            adapter_suffix, adapter_suffix);
    fprintf(tcl_script,
            "create_bd_cell -type module -reference address_adapter%s "
            "address_adapter%s\n",
            adapter_suffix, adapter_suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_addr_a] "
            "[get_bd_pins address_adapter%s/axi_addr]\n",
            name, suffix, adapter_suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins address_adapter%s/bram_addr] "
            "[get_bd_pins blk_mem_gen_%s_%s/addra]\n",
            adapter_suffix, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_wrdata_a] "
            "[get_bd_pins blk_mem_gen_%s_%s/dina]\n",
            name, suffix, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_rddata_a] "
            "[get_bd_pins blk_mem_gen_%s_%s/douta]\n",
            name, suffix, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_en_a] "
            "[get_bd_pins blk_mem_gen_%s_%s/ena]\n",
            name, suffix, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_we_a] "
            "[get_bd_pins write_enb_adapter%s/axi_write_enb]\n",
            name, suffix, adapter_suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins write_enb_adapter%s/bram_write_enb] "
            "[get_bd_pins blk_mem_gen_%s_%s/wea]\n",
            adapter_suffix, name, suffix);
    fprintf(tcl_script,
            "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_clk_a] "
            "[get_bd_pins blk_mem_gen_%s_%s/clka]\n",
            name, suffix, name, suffix);

    return ERR_NONE;
}

auto_error_t create_mem_interface(size_t array_size,
                                  const char *filename_suffix,
                                  size_t array_width) {

    char filename[MAX_NAME_LENGTH];
    memset(filename, 0, MAX_NAME_LENGTH * sizeof(char));

    int written = snprintf(filename, MAX_NAME_LENGTH, "mem_interface%s.vhd",
                           filename_suffix);
    CHECK_LENGTH(written, MAX_NAME_LENGTH)

    FILE *hdl_mem_interface = fopen(filename, "w");
    CHECK_NULL(hdl_mem_interface, ERR_IO,
               "Couldn't create file mem_interface !")

    *strrchr(filename, '.') = '\0';

    fprintf(hdl_mem_interface, "library IEEE;\n");
    fprintf(hdl_mem_interface, "use IEEE.STD_LOGIC_1164.ALL;\n");
    fprintf(hdl_mem_interface, "entity mem_interface%s is\n", filename_suffix);

    uint16_t nb_bytes_in_width = (uint16_t) (array_width >> 3);
    int msb_axi = 64 - __builtin_clzll(array_size) - 2;

    size_t nb_elem = array_size / (array_width >> 3);
    int msb_bram = 64 - __builtin_clzll(nb_elem) - 2;

    fprintf(hdl_mem_interface,
            "port (\taxi_bram_addr : in std_logic_vector(%d downto 0);\n",
            msb_axi);
    fprintf(hdl_mem_interface,
            "\t\taxi_bram_wrdata : in std_logic_vector(%zu downto 0);\n",
            array_width - 1);
    fprintf(hdl_mem_interface,
            "\t\taxi_bram_rddata : out std_logic_vector(%zu downto 0);\n",
            array_width - 1);
    fprintf(hdl_mem_interface, "\t\taxi_bram_en : in std_logic;\n");
    fprintf(hdl_mem_interface,
            "\t\taxi_bram_wen : in std_logic_vector(%d downto 0);\n",
            nb_bytes_in_width - 1);
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface,
            "\t\tdyn_bram_addr : in std_logic_vector(31 downto 0);\n");
    fprintf(hdl_mem_interface,
            "\t\tdyn_bram_wrdata : in std_logic_vector(%zu downto 0);\n",
            array_width - 1);
    fprintf(hdl_mem_interface,
            "\t\tdyn_bram_rddata : out std_logic_vector(%zu downto 0);\n",
            array_width - 1);
    fprintf(hdl_mem_interface, "\t\tdyn_bram_en : in std_logic;\n");

    fprintf(hdl_mem_interface,
            "\t\tmem_bram_addr : out std_logic_vector(31 downto 0);\n");
    fprintf(hdl_mem_interface,
            "\t\tmem_bram_wrdata : out std_logic_vector(%zu downto 0);\n",
            array_width - 1);
    fprintf(hdl_mem_interface,
            "\t\tmem_bram_rddata : in std_logic_vector(%zu downto 0);\n",
            array_width - 1);
    fprintf(hdl_mem_interface, "\t\tmem_bram_en : out std_logic;\n");
    fprintf(hdl_mem_interface, "\t\tmem_bram_wen : out std_logic);\n");
    fprintf(hdl_mem_interface, "\t\tend mem_interface%s;\n", filename_suffix);
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface,
            "architecture Behavioral of mem_interface%s is\n", filename_suffix);
    fprintf(hdl_mem_interface, "\n");
    fprintf(hdl_mem_interface, "\tcomponent address_adapter%s is\n",
            filename_suffix);
    fprintf(hdl_mem_interface,
            "\t\tport ( axi_addr : in std_logic_vector(%d downto 0);\n",
            msb_axi);
    fprintf(hdl_mem_interface,
            "\t\tbram_addr : out std_logic_vector(%d downto 0));\n", msb_bram);
    fprintf(hdl_mem_interface, "\t\tend component address_adapter%s;\n",
            filename_suffix);
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "\tcomponent write_enb_adapter%s is\n",
            filename_suffix);
    fprintf(hdl_mem_interface,
            "\t\tport ( axi_write_enb : in std_logic_vector(%d downto 0);\n",
            nb_bytes_in_width - 1);
    fprintf(hdl_mem_interface, "\t\tbram_write_enb : out std_logic);\n");
    fprintf(hdl_mem_interface, "\tend component write_enb_adapter%s;\n",
            filename_suffix);
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface,
            "\tsignal axi_adapted_addr : std_logic_vector(%d downto 0);\n",
            msb_bram);
    fprintf(hdl_mem_interface, "\tsignal axi_adapted_wen : std_logic;\n");
    fprintf(
            hdl_mem_interface,
            "\tsignal axi_adapted_addr_to_32 : std_logic_vector(31 downto 0);\n");

    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "begin\n");
    fprintf(hdl_mem_interface, "\n");
    fprintf(hdl_mem_interface,
            "\taxi_adapted_addr_to_32(%d downto 0) <= axi_adapted_addr;\n",
            msb_bram);
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "\taddress_adapter_inst_i : address_adapter%s\n",
            filename_suffix);
    fprintf(hdl_mem_interface, "\t\tport map (\n");
    fprintf(hdl_mem_interface, "\t\t\taxi_addr => axi_bram_addr,\n");
    fprintf(hdl_mem_interface, "\t\t\tbram_addr => axi_adapted_addr\n");
    fprintf(hdl_mem_interface, "\t\t);\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface,
            "\twrite_enb_adapter_inst_i : write_enb_adapter%s\n",
            filename_suffix);
    fprintf(hdl_mem_interface, "\t\tport map (\n");
    fprintf(hdl_mem_interface, "\t\t\taxi_write_enb => axi_bram_wen,\n");
    fprintf(hdl_mem_interface, "\t\t\tbram_write_enb => axi_adapted_wen\n");
    fprintf(hdl_mem_interface, "\t\t);\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(
            hdl_mem_interface,
            "\tmem_bram_addr <= axi_adapted_addr_to_32 when dyn_bram_en = '0' ");
    fprintf(hdl_mem_interface,
            "or (axi_adapted_wen = '1' and axi_bram_en = '1') else\n");
    fprintf(hdl_mem_interface, "\t\t\t\t\tdyn_bram_addr;\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface,
            "\tmem_bram_wrdata <= axi_bram_wrdata when dyn_bram_en = '0' ");
    fprintf(hdl_mem_interface,
            "or (axi_adapted_wen = '1' and axi_bram_en = '1') else\n");
    fprintf(hdl_mem_interface, "\t\t\t\t\tdyn_bram_wrdata;\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "\taxi_bram_rddata <= mem_bram_rddata;\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "\tdyn_bram_rddata <= mem_bram_rddata;\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface,
            "\tmem_bram_en <= axi_bram_en when dyn_bram_en = '0' ");
    fprintf(hdl_mem_interface,
            "or (axi_adapted_wen = '1' and axi_bram_en = '1') else\n");
    fprintf(hdl_mem_interface, "\t\t\t\t\tdyn_bram_en;\n");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "\tmem_bram_wen <= '1' when axi_adapted_wen = "
                               "'1' and axi_bram_en = '1' else '0';");
    fprintf(hdl_mem_interface, "\n");

    fprintf(hdl_mem_interface, "end Behavioral;\n");

    fclose(hdl_mem_interface);

    return ERR_NONE;
}

auto_error_t generate_memory(FILE *tcl_script, hdl_array_t *arr,
                             axi_ip_t *axi_ip, const char *exec_path,
                             size_t array_size) {
    CHECK_PARAM(tcl_script)
    CHECK_PARAM(arr)
    CHECK_PARAM(axi_ip)

    char filename_suffix[MAX_NAME_LENGTH];
    memset(filename_suffix, 0, MAX_NAME_LENGTH * sizeof(char));

    if (arr->read && arr->write) {

        char *name = arr->name;
        const char *suffix = "read_write";

        // Create mem and axi controller

        create_bram(tcl_script, name, suffix, array_size, arr->width);
        create_axi_bram_controller(tcl_script, name, suffix, arr->width);

        // Write

        bram_interface_t *interface = &(arr->write_ports);

        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "blk_mem_gen_%s_%s/addrb]\n",
                axi_ip->name, interface->address, name, suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "blk_mem_gen_%s_%s/enb]\n",
                axi_ip->name, interface->ce, name, suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "blk_mem_gen_%s_%s/web]\n",
                axi_ip->name, interface->we, name, suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "blk_mem_gen_%s_%s/doutb]\n",
                axi_ip->name, interface->din, name, suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "blk_mem_gen_%s_%s/dinb]\n",
                axi_ip->name, interface->dout, name, suffix);

        int written = snprintf(filename_suffix, MAX_NAME_LENGTH, "_%s_%s",
                               arr->name, "read_write");
        CHECK_LENGTH(written, MAX_NAME_LENGTH)

        CHECK_CALL_DO(generate_adapters(tcl_script, exec_path, arr, filename_suffix,
                                        array_size),
                      "generate_adapters failed !", fclose(tcl_script);)

        // Read
        CHECK_CALL_DO(
                create_mem_interface(array_size, filename_suffix, arr->width),
                "create_mem_interface failed !", fclose(tcl_script);)

        interface = &(arr->read_ports);

        fprintf(tcl_script,
                "import_files -norecurse {%s/mem_interface%s.vhd}\n",
                exec_path, filename_suffix);
        fprintf(tcl_script, "update_compile_order -fileset sources_1\n");
        fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

        fprintf(tcl_script,
                "create_bd_cell -type module -reference mem_interface%s "
                "mem_interface%s_0\n",
                filename_suffix, filename_suffix);

        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_addr_a] "
                "[get_bd_pins mem_interface%s_0/axi_bram_addr]\n",
                arr->name, suffix, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_clk_a] "
                "[get_bd_pins blk_mem_gen_%s_%s/clka]\n",
                arr->name, suffix, name, suffix);
        fprintf(
                tcl_script,
                "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_wrdata_a] "
                "[get_bd_pins mem_interface%s_0/axi_bram_wrdata]\n",
                arr->name, suffix, filename_suffix);
        fprintf(
                tcl_script,
                "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_rddata_a] "
                "[get_bd_pins mem_interface%s_0/axi_bram_rddata]\n",
                arr->name, suffix, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_en_a] "
                "[get_bd_pins mem_interface%s_0/axi_bram_en]\n",
                arr->name, suffix, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins axi_bram_ctrl_%s_%s/bram_we_a] "
                "[get_bd_pins mem_interface%s_0/axi_bram_wen]\n",
                arr->name, suffix, filename_suffix);

        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "mem_interface%s_0/dyn_bram_addr]\n",
                axi_ip->name, interface->address, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "mem_interface%s_0/dyn_bram_en]\n",
                axi_ip->name, interface->ce, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "mem_interface%s_0/dyn_bram_wrdata]\n",
                axi_ip->name, interface->dout, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins %s_0/dynamatic_%s] [get_bd_pins "
                "mem_interface%s_0/dyn_bram_rddata]\n",
                axi_ip->name, interface->din, filename_suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins mem_interface%s_0/mem_bram_addr] "
                "[get_bd_pins blk_mem_gen_%s_%s/addra]\n",
                filename_suffix, arr->name, suffix);
        fprintf(
                tcl_script,
                "connect_bd_net [get_bd_pins mem_interface%s_0/mem_bram_wrdata] "
                "[get_bd_pins blk_mem_gen_%s_%s/dina]\n",
                filename_suffix, arr->name, suffix);
        fprintf(
                tcl_script,
                "connect_bd_net [get_bd_pins mem_interface%s_0/mem_bram_rddata] "
                "[get_bd_pins blk_mem_gen_%s_%s/douta]\n",
                filename_suffix, arr->name, suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins mem_interface%s_0/mem_bram_en] "
                "[get_bd_pins blk_mem_gen_%s_%s/ena]\n",
                filename_suffix, arr->name, suffix);
        fprintf(tcl_script,
                "connect_bd_net [get_bd_pins mem_interface%s_0/mem_bram_wen] "
                "[get_bd_pins blk_mem_gen_%s_%s/wea]\n",
                filename_suffix, arr->name, suffix);
    } else if (arr->write) {
        int written = snprintf(filename_suffix, MAX_NAME_LENGTH, "_%s_%s",
                               arr->name, "write");
        CHECK_LENGTH(written, MAX_NAME_LENGTH)

        CHECK_CALL_DO(generate_adapters(tcl_script, exec_path, arr, filename_suffix,
                                        array_size),
                      "generate_adapters failed !", fclose(tcl_script);)
        CHECK_CALL(generate_memory_interface(tcl_script, axi_ip, arr->name,
                                             "write", &(arr->write_ports),
                                             array_size, filename_suffix,
                                             arr->width),
                   "generate_memory_interface failed !")
    } else if (arr->read) {
        int written = snprintf(filename_suffix, MAX_NAME_LENGTH, "_%s_%s",
                               arr->name, "read");
        CHECK_LENGTH(written, MAX_NAME_LENGTH)

        CHECK_CALL_DO(generate_adapters(tcl_script, exec_path, arr, filename_suffix,
                                        array_size),
                      "generate_adapters failed !", fclose(tcl_script);)
        CHECK_CALL(generate_memory_interface(tcl_script, axi_ip, arr->name,
                                             "read", &(arr->read_ports),
                                             array_size, filename_suffix,
                                             arr->width),
                   "generate_memory_interface failed !")
    }
    return ERR_NONE;
}

auto_error_t generate_final_script(project_t *project, vivado_hls_t *hls,
                                   axi_ip_t *axi_ip, const char *part) {
    CHECK_PARAM(project)
    CHECK_PARAM(project->hdl_info)
    CHECK_PARAM(project->hdl_info->arrays)
    CHECK_PARAM(hls)

    FILE *tcl_script = fopen("final_script.tcl", "w");
    CHECK_NULL(tcl_script, ERR_IO, "Could not open file : final_script.tcl")

    fprintf(tcl_script, "open_project %s/edit_%s_v1_0.xpr\n", axi_ip->path,
            axi_ip->name);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    if (hls->nb_float_op > 0) {
        fprintf(tcl_script, "add_files -norecurse -copy_to %s/%s_1.0/src {",
                axi_ip->path, axi_ip->name);

        for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
            float_op_t *op = &(hls->float_ops[i]);
            if (i == hls->nb_float_op - 1) {
                fprintf(tcl_script, "%s", op->hdl_path);
            } else {
                fprintf(tcl_script, "%s ", op->hdl_path);
            }
        }
        fprintf(tcl_script, "}\n");

        for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
            float_op_t *op = &(hls->float_ops[i]);
            fprintf(tcl_script, "source %s\n", op->script_path);
        }
    }

    fprintf(tcl_script, "ipx::open_ipxact_file %s/%s_1.0/component.xml\n",
            axi_ip->path, axi_ip->name);

    fprintf(tcl_script,
            "set_property widget {textEdit} [ipgui::get_guiparamspec -name "
            "\"C_%s_DATA_WIDTH\" -component [ipx::current_core] ]\n",
            axi_ip->interface_name);
    fprintf(tcl_script,
            "set_property value_validation_type none [ipx::get_user_parameters "
            "C_%s_DATA_WIDTH -of_objects [ipx::current_core]]\n",
            axi_ip->interface_name);
    fprintf(tcl_script,
            "set_property enablement_value true [ipx::get_user_parameters "
            "C_%s_DATA_WIDTH -of_objects [ipx::current_core]]\n",
            axi_ip->interface_name);
    fprintf(tcl_script,
            "set_property widget {textEdit} [ipgui::get_guiparamspec -name "
            "\"C_%s_ADDR_WIDTH\" -component [ipx::current_core] ]\n",
            axi_ip->interface_name);
    fprintf(tcl_script,
            "set_property enablement_value true [ipx::get_user_parameters "
            "C_%s_ADDR_WIDTH -of_objects [ipx::current_core]]\n",
            axi_ip->interface_name);
    fprintf(tcl_script,
            "set_property value_validation_type none [ipx::get_user_parameters "
            "C_%s_ADDR_WIDTH -of_objects [ipx::current_core]]\n",
            axi_ip->interface_name);

    fprintf(tcl_script,
            "ipx::merge_project_changes hdl_parameters [ipx::current_core]\n");

    axi_ip->revision = axi_ip->revision + 1;
    fprintf(tcl_script, "set_property core_revision %d [ipx::current_core]\n",
            axi_ip->revision);
    fprintf(tcl_script, "ipx::create_xgui_files [ipx::current_core]\n");
    fprintf(tcl_script, "ipx::update_checksums [ipx::current_core]\n");
    fprintf(tcl_script, "ipx::save_core [ipx::current_core]\n");

    fprintf(tcl_script, "update_ip_catalog -rebuild -repo_path %s/%s_1.0\n",
            axi_ip->path, axi_ip->name);
    fprintf(tcl_script, "close_project\n");

    fprintf(tcl_script, "open_project %s/%s/%s.xpr\n", project->path,
            project->name, project->name);
    fprintf(tcl_script, "update_ip_catalog -rebuild\n");

    //Not working
    if (strcmp(part, "xcvu9p-flgb2104-2-i") == 0) {
        fprintf(tcl_script, "aws::make_ipi\n");
        fprintf(tcl_script, "set_property ip_repo_paths [concat [get_property "
                            "ip_repo_paths [current_project]] ");
        fprintf(tcl_script, "{%s/ip_repo}] [current_project]\n", project->path);
        // https://forums.xilinx.com/t5/Vivado-TCL-Community/append-to-existing-ip-repo-paths/td-p/742487
        // set_property ip_repo_paths [concat [get_property ip_repo_paths
        // [current_project]] {c:/new_repo/}] [current_project]
    } else {
        fprintf(tcl_script, "create_bd_design \"design_1\"\n");
    }
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");

    fprintf(tcl_script, "startgroup\n");
    fprintf(tcl_script,
            "create_bd_cell -type ip -vlnv user.org:user:%s:1.0 %s_0\n",
            axi_ip->name, axi_ip->name);
    fprintf(tcl_script, "endgroup\n");

    size_t array_size = (size_t) (project->array_size);
    size_t ind_to_num;
    char ind = project->array_size_ind;
    if (ind == 'K') {
        ind_to_num = 10;
    } else if (ind == 'M') {
        ind_to_num = 20;
    } else {
        ind_to_num = 30;
    }
    array_size = array_size << ind_to_num;

    for (size_t i = 0; i < project->hdl_info->nb_arrays; ++i) {
        CHECK_CALL_DO(generate_memory(tcl_script,
                                      &(project->hdl_info->arrays[i]), axi_ip,
                                      project->exec_path, array_size),
                      "generate_memory failed !", fclose(tcl_script);)
    }

    script_func_t script_func = select_part_script(part);
    // script_func = NULL;
    if (script_func != NULL) {
        script_func(tcl_script, project, axi_ip);
    }

    fprintf(tcl_script, "save_bd_design\n");

    fclose(tcl_script);

    return ERR_NONE;
}

auto_error_t generate_hls_script(vivado_hls_t *hls, const char *part) {
    CHECK_PARAM(hls)

    FILE *tcl_script = fopen("hls_script.tcl", "w");
    CHECK_NULL(tcl_script, ERR_IO, "Could not open file : hls_script.tcl")

    fprintf(tcl_script, "open_project -reset vivado_hls\n");
    fprintf(tcl_script, "set_top %s\n", hls->fun_name);

    char dir_path[MAX_PATH_LENGTH];
    memset(dir_path, 0, MAX_PATH_LENGTH * sizeof(char));

    strncpy(dir_path, hls->source_path, MAX_PATH_LENGTH);

    char *last_slash = strrchr(dir_path, '/');
    CHECK_COND_DO(last_slash == NULL, ERR_NULL,
                  "Wrong path for dir of source C/C++ file !",
                  fclose(tcl_script);)
    *last_slash = '\0';

    char main_name[MAX_NAME_LENGTH];
    memset(main_name, 0, MAX_NAME_LENGTH * sizeof(char));

    last_slash = strrchr(hls->source_path, '/');
    char *start_cpy;
    if (last_slash == NULL) {
        start_cpy = hls->source_path;
    } else {
        start_cpy = last_slash + 1;
    }
    CHECK_COND(strlen(start_cpy) > MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
               "Name too long !")
    strncpy(main_name, start_cpy, MAX_NAME_LENGTH);
    *strrchr(main_name, '.') = '\0';
    char *name_part = strrchr(main_name, '_');
    if (name_part != NULL) {
        *name_part = '\0';
    }

    char full_cpp_name[MAX_NAME_LENGTH];
    memset(full_cpp_name, 0, MAX_NAME_LENGTH * sizeof(char));

    strncpy(full_cpp_name, start_cpy, MAX_NAME_LENGTH);

    char cpp_name_pattern[MAX_NAME_LENGTH];
    memset(cpp_name_pattern, 0, MAX_NAME_LENGTH * sizeof(char));

    snprintf(cpp_name_pattern, MAX_NAME_LENGTH, "%s", main_name);

    CHECK_CALL(set_pattern(cpp_name_pattern, 1), "set_pattern failed !")

    regmatch_t match[1];

    CHECK_CALL(set_pattern("\\w+.(cpp|h|c)", 2), "set_pattern failed !")

    DIR *d;
    d = opendir(dir_path);
    CHECK_COND_DO(d == NULL, ERR_IO, "Could not open dir !",
                  fclose(tcl_script);)

    char **files_to_add;
    uint8_t count = 0;
    uint8_t last = 0;
    CHECK_CALL_DO(allocate_str_arr(&files_to_add, &last),
                  "allocate_str_arr failed !", fclose(tcl_script);closedir(d);)

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        auto_error_t err = find_set_pattern(dir->d_name, 1, match, 2);
        if (count == last) {
            CHECK_CALL_DO(allocate_str_arr(&files_to_add, &last),
                          "allocate_str_arr failed !", fclose(tcl_script);
                                  closedir(d);
                                  free_str_arr(files_to_add, last);)
        }
        if (err == ERR_NONE) {
            auto_error_t cpp_err = find_set_pattern(dir->d_name, 1, match, 1);
            if (cpp_err != ERR_NONE) {
                CHECK_LENGTH(strlen(dir->d_name), MAX_NAME_LENGTH)
                strncpy(files_to_add[count++], dir->d_name,
                        MAX_NAME_LENGTH);
            } else {
                if (strcmp(dir->d_name, full_cpp_name) == 0 ||
                    strcmp(".h", strrchr(dir->d_name, '.')) == 0) {
                    strncpy(files_to_add[count++], dir->d_name,
                            MAX_NAME_LENGTH);
                }
            }
        }
    }
    closedir(d);

    // Files to
    // add----------------------------------------------------------------------------

    for (uint8_t i = 0; i < count; ++i) {
        fprintf(tcl_script, "add_files %s/%s\n", dir_path, files_to_add[i]);
    }

    free_str_arr(files_to_add, last);

    fprintf(tcl_script, "open_solution -reset \"solution1\"\n");

    if (strcmp(part, "xcvu9p-flgb2104-2-i") == 0) {
        fprintf(tcl_script, "set_part {xc7z045ffg900-2}\n");
    } else {
        fprintf(tcl_script, "set_part {%s}\n", part);
    }
    fprintf(tcl_script, "create_clock -period 100MHz\n");

    fprintf(tcl_script, "csynth_design\n");

    fprintf(tcl_script, "export_design -format ip_catalog -rtl vhdl\n");

    fclose(tcl_script);

    return ERR_NONE;
}
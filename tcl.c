
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcl.h"
#include "hdl_data.h"

void create_AXI_script(hdl_source_t* hdl_source) {
    axi_ip_t* ip = malloc(sizeof(axi_ip_t));
    if(ip == NULL) {
        fprintf(stderr, "Failed to allocate memory for axi_ip !\n");
    }
    strcpy(ip->name, "axi_ip_dynamatic_test");
    strcpy(ip->path, "ip_repo");
    strcpy(ip->interface_name, "CSR");
    ip->nb_slave_registers = hdl_source->nb_params + 2;

    FILE* tcl_script = fopen("generate_axi_ip.tcl", "w");
    if(tcl_script == NULL) {
        fclose(tcl_script);
        fprintf(stderr, "The program did not manage to create the script !\n");
        return;
    }
    fprintf(tcl_script, "create_peripheral user.org user %s 1.0 -dir ip_repo\n", ip->name);
    fprintf(tcl_script, "add_peripheral_interface %s -interface_mode slave -axi_type lite [ipx::find_open_core user.org:user:%s:1.0]\n",
    ip->interface_name, ip->name);
    fprintf(tcl_script, "set_property VALUE %zu [ipx::get_bus_parameters WIZ_NUM_REG -of_objects [ipx::get_bus_interfaces %s -of_objects [ipx::find_open_core user.org:user:%s:1.0]]]\n",
    ip->nb_slave_registers, ip->interface_name, ip->name);
    fprintf(tcl_script, "generate_peripheral -bfm_example_design -debug_hw_example_design [ipx::find_open_core user.org:user:%s:1.0]\n", ip->name);
    fprintf(tcl_script, "write_peripheral [ipx::find_open_core user.org:user:%s:1.0]\n", ip->name);
    fprintf(tcl_script, "set_property  ip_repo_paths  %s/%s_1.0 [current_project]\n", ip->path, ip->name);
    fprintf(tcl_script, "update_ip_catalog -rebuild\n");
    fprintf(tcl_script, "ipx::edit_ip_in_project -upgrade true -name edit_%s_v1_0 -directory %s %s/%s_1.0/component.xml\n",
    ip->name, ip->path, ip->path, ip->name);
    fprintf(tcl_script, "update_compile_order -fileset sources_1\n");
    fclose(tcl_script);
}
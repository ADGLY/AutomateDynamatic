
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tcl.h"
#include "hdl_data.h"



void get_project_path(project_t* project) {
    printf("What is the path of the Vivado project ?\n");
    char temp[MAX_NAME_LENGTH];
    char* result = fgets(temp, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    if(temp[0] != '/') {
        char* result = getcwd(project->path, MAX_NAME_LENGTH);
        if(result == NULL) {
            fprintf(stderr, "getcwd error !\n");
            return;
        }
        if(temp[0] != '.') {
            project->path[strlen(project->path)] = '/';
            strcpy(project->path + strlen(project->path), temp);
        }
    }
    project->path[strlen(project->path) - 1] = '\0';

    return;
}

void create_project(project_t* project, hdl_source_t* hdl_source) {
    memset(project, 0, sizeof(project_t));
    get_project_path(project);
    strcpy(project->axi_ip.path, project->path);
    strcpy(project->axi_ip.path + strlen(project->axi_ip.path), "/ip_repo");
    project->hdl_source = hdl_source;
}



void generate_AXI_script(project_t* project) {
    axi_ip_t* axi_ip = &(project->axi_ip);
    strcpy(axi_ip->name, "axi_ip_dynamatic_test");
    strcpy(axi_ip->interface_name, "CSR");
    axi_ip->nb_slave_registers = project->hdl_source->nb_params + 2;

    FILE* tcl_script = fopen("generate_axi_ip.tcl", "w");
    if(tcl_script == NULL) {
        fclose(tcl_script);
        fprintf(stderr, "The program did not manage to create the script !\n");
        return;
    }
    fprintf(tcl_script, "create_peripheral user.org user %s 1.0 -dir ip_repo\n", axi_ip->name);
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
    fclose(tcl_script);
}

void free_axi_ip(axi_ip_t* axi_ip) {
    if(axi_ip == NULL) {
        return;
    }
    free(axi_ip);
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include "tcl.h"
#include "hdl.h"



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

void get_project_name(project_t* project) {
    printf("What is the name of the Vivado project ?\n");
    char* result = fgets(project->name, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    project->name[strlen(project->name) - 1] = '\0';

    return;
}

void create_project(project_t* project, hdl_source_t* hdl_source) {
    memset(project, 0, sizeof(project_t));
    get_project_path(project);
    get_project_name(project);
    strcpy(project->axi_ip.path, project->path);
    strcpy(project->axi_ip.path + strlen(project->axi_ip.path), "/ip_repo");
    project->hdl_source = hdl_source;
}

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
    strcpy(ip_script_path + strlen(ip_script_path), "/generate_axi_ip.tcl");

    fprintf(tcl_script, "source %s", ip_script_path);

    fclose(tcl_script);


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
     //add_files -norecurse -copy_to /home/antoine/ip_repo/dynamatic_test_1.0/src {/home/antoine/Documents/Dynamatic/Histo_187/hdl/LSQ_hist.v /home/antoine/Documents/Dynamatic/Histo_187/hdl/mul_wrapper.vhd /home/antoine/Documents/Dynamatic/Histo_187/hdl/arithmetic_units.vhd /home/antoine/Documents/Dynamatic/Histo_187/hdl/elastic_components.vhd /home/antoine/Documents/Dynamatic/Histo_187/hdl/multipliers.vhd /home/antoine/Documents/Dynamatic/Histo_187/hdl/MemCont.vhd /home/antoine/Documents/Dynamatic/Histo_187/hdl/histogram_elaborated_optimized.vhd /home/antoine/Documents/Dynamatic/Histo_187/hdl/delay_buffer.vhd}
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

    //set_property file_type {VHDL 2008} [get_files  /home/antoine/ip_repo/dynamatic_test_1.0/src/mul_wrapper.vhd]
    for(int i = 0; i < nb_files; ++i) {
        if(files_to_add[i][strlen(files_to_add[i]) - 1] == 'd') {
            fprintf(tcl_script, "set_property file_type {VHDL 2008} [get_files  ");
            fprintf(tcl_script, "%s/%s_1.0/src/%s]\n", axi_ip->path, axi_ip->name, files_to_add[i]);
        }
    }




    fclose(tcl_script);
}

void free_axi_ip(axi_ip_t* axi_ip) {
    if(axi_ip == NULL) {
        return;
    }
    free(axi_ip);
}
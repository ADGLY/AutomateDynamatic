#include <regex.h>
#include <stdio.h>
#include "axi_files.h"
#include "file.h"
#include "hdl_data.h"


void read_axi_files(axi_ip_t* axi_ip) {
    char* top_file = get_source("axi_ip_dynamatic_test_v1_0.vhd");
    char* axi_file = get_source("axi_ip_dynamatic_test_v1_0_CSR.vhd");
    axi_ip->axi_files.axi_file = axi_file;
    axi_ip->axi_files.top_file = top_file;
}

void write_array_ports(FILE* new_top_file, bram_interface_t* interface) {
    const char* prefix = "dynamatic_";
    
    fprintf(new_top_file, "\t\t%s", prefix);
    fprintf(new_top_file, "%s", interface->address);
    fprintf(new_top_file, "%s\n", " : out std_logic_vector (31 downto 0);");


    fprintf(new_top_file, "\t\t%s", prefix);
    fprintf(new_top_file, "%s", interface->ce);
    fprintf(new_top_file, "%s\n", " : out std_logic;");

    fprintf(new_top_file, "\t\t%s", prefix);
    fprintf(new_top_file, "%s", interface->we);
    fprintf(new_top_file, "%s\n", " : out std_logic;");

    fprintf(new_top_file, "\t\t%s", prefix);
    fprintf(new_top_file, "%s", interface->dout);
    fprintf(new_top_file, "%s\n", " : out std_logic_vector (31 downto 0);");

    fprintf(new_top_file, "\t\t%s", prefix);
    fprintf(new_top_file, "%s", interface->din);
    fprintf(new_top_file, "%s\n", " : in std_logic_vector (31 downto 0);");
}

void write_array_ports_wo_prefix(FILE* new_top_file, bram_interface_t* interface) {
    fprintf(new_top_file, "\t\t%s", interface->address);
    fprintf(new_top_file, "%s\n", " : out std_logic_vector (31 downto 0);");

    fprintf(new_top_file, "\t\t%s", interface->ce);
    fprintf(new_top_file, "%s\n", " : out std_logic;");

    fprintf(new_top_file, "\t\t%s", interface->we);
    fprintf(new_top_file, "%s\n", " : out std_logic;");

    fprintf(new_top_file, "\t\t%s", interface->dout);
    fprintf(new_top_file, "%s\n", " : out std_logic_vector (31 downto 0);");

    fprintf(new_top_file, "\t\t%s", interface->din);
    fprintf(new_top_file, "%s\n", " : in std_logic_vector (31 downto 0);");
}

void update_files(project_t* project) {
    regex_t reg;
    regmatch_t match[1];

    FILE* new_top_file = fopen("top_file.tmp", "a+");

    char* top_file_off = project->axi_ip.axi_files.top_file;

    int err = regcomp(&reg, "-- Users to add ports here", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, top_file_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }

    fwrite(top_file_off, sizeof(char), match[0].rm_eo + 1, new_top_file);
    top_file_off += match[0].rm_eo + 1;

    hdl_source_t* hdl_source = project->hdl_source;
    
    for(int i = 0; i < hdl_source->nb_arrays; ++i) {
      write_array_ports(new_top_file, &(hdl_source->arrays[i].write_ports));
      write_array_ports(new_top_file, &(hdl_source->arrays[i].read_ports));  
    }


    err = regcomp(&reg, "port [(]", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, top_file_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }

    fwrite(top_file_off, sizeof(char), match[0].rm_eo + 1, new_top_file);
    top_file_off += match[0].rm_eo + 1;

    fprintf(new_top_file, "\n");
    fprintf(new_top_file, "\t\taxi_start_valid : out std_logic;\n");
    fprintf(new_top_file, "\t\taxi_start_ready : in std_logic;\n");
    fprintf(new_top_file, "\t\taxi_end_out : in std_logic_vector(0 downto 0);\n");
    fprintf(new_top_file, "\t\taxi_end_valid : in std_logic;\n");
    fprintf(new_top_file, "\t\taxi_end_ready : out std_logic;\n");

    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\t\taxi_%s_din : out std_logic_vector(31 downto 0);\n", param_name);
    }

    fprintf(new_top_file, "\t\taxi_reset : out std_logic;\n");


    err = regcomp(&reg, "end component \\w+;", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, top_file_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }

    fwrite(top_file_off, sizeof(char), match[0].rm_eo + 1, new_top_file);
    top_file_off += match[0].rm_eo + 1;

    fprintf(new_top_file, "\n");

    fprintf(new_top_file, "\tcomponent %s is\n", hdl_source->name);
    fprintf(new_top_file, "\t\tport (\n");

    fprintf(new_top_file, "\t\tclk:  in std_logic;\n");
    fprintf(new_top_file, "\t\trst:  in std_logic;\n");
    fprintf(new_top_file, "\t\tstart_in:  in std_logic_vector (0 downto 0);\n");
    fprintf(new_top_file, "\t\tstart_valid:  in std_logic;\n");
    fprintf(new_top_file, "\t\tstart_ready:  out std_logic;\n");
    fprintf(new_top_file, "\t\tend_out:  out std_logic_vector (0 downto 0);\n");
    fprintf(new_top_file, "\t\tend_valid:  out std_logic;\n");
    fprintf(new_top_file, "\t\tend_ready:  in std_logic;\n");

    for(int i = 0; i < hdl_source->nb_arrays; ++i) {
      write_array_ports_wo_prefix(new_top_file, &(hdl_source->arrays[i].write_ports));
      write_array_ports_wo_prefix(new_top_file, &(hdl_source->arrays[i].read_ports));  
    }

    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\t\t%s_din : in std_logic_vector (31 downto 0);\n", param_name);
        fprintf(new_top_file, "\t\t%s_valid_in : in std_logic;\n", param_name);
        if(i == hdl_source->nb_params - 1) {
            fprintf(new_top_file, "\t\t%s_ready_out : out std_logic\n", param_name);
        }
        else {
            fprintf(new_top_file, "\t\t%s_ready_out : out std_logic\n", param_name);
        }
    }

    fprintf(new_top_file, "\t\t);\n");
    fprintf(new_top_file, "\tend component %s;\n", hdl_source->name);

    fprintf(new_top_file, "\n");

    fprintf(new_top_file, "\tsignal dynamatic_rst : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_start_valid : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_start_ready : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_end_out : std_logic_vector(0 downto 0);\n");
    fprintf(new_top_file, "\tsignal dynamatic_end_valid : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_end_ready : std_logic;\n");

    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\tsignal dynamatic_%s_din : std_logic_vector(31 downto 0);\n", param_name);
    }

    err = regcomp(&reg, "port map [(]", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, top_file_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }

    fwrite(top_file_off, sizeof(char), match[0].rm_eo + 1, new_top_file);
    top_file_off += match[0].rm_eo + 1;

    fprintf(new_top_file, "\t\taxi_start_valid => dynamatic_start_valid,\n");
    fprintf(new_top_file, "\t\taxi_start_ready => dynamatic_start_ready,\n");
    fprintf(new_top_file, "\t\taxi_end_out => dynamatic_end_out,\n");
    fprintf(new_top_file, "\t\taxi_end_valid => dynamatic_end_valid,\n");
    fprintf(new_top_file, "\t\taxi_end_ready => dynamatic_end_ready,\n");

    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\t\taxi_%s_din => dynamatic_%s_din,\n", param_name, param_name);
    }

    fprintf(new_top_file, "\t\taxi_reset => dynamatic_rst,\n");

    err = regcomp(&reg, "-- Add user logic here", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, top_file_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }

    fwrite(top_file_off, sizeof(char), match[0].rm_eo + 1, new_top_file);
    top_file_off += match[0].rm_eo + 1;

    fprintf(new_top_file, "\t%s_inst : %s\n", hdl_source->name, hdl_source->name);

    fclose(new_top_file);
}
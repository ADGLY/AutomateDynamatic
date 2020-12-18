#include <regex.h>
#include <stdio.h>
#include <string.h>
#include "axi_files.h"
#include "file.h"
#include "hdl_data.h"


void read_axi_files(axi_ip_t* axi_ip) {

    char top_file_path[MAX_NAME_LENGTH];
    char top_file_path_suffix[MAX_NAME_LENGTH];
    strcpy(top_file_path, axi_ip->path);
    sprintf(top_file_path_suffix, "/%s_1.0/hdl/%s_v1.0.vhd", axi_ip->name,
     axi_ip->name);
    strcpy(top_file_path + strlen(axi_ip->path), top_file_path_suffix);

    strcpy(top_file_path, axi_ip->axi_files.top_file_path);
    
    char* top_file = get_source(top_file_path);

    char axi_path[MAX_NAME_LENGTH];
    char axi_path_suffix[MAX_NAME_LENGTH];
    strcpy(axi_path, axi_ip->path);
    sprintf(axi_path_suffix, "/%s_1.0/hdl/%s_v1.0.vhd", axi_ip->name,
     axi_ip->name);
    strcpy(axi_path + strlen(axi_ip->path), axi_path_suffix);

    strcpy(axi_path, axi_ip->axi_files.axi_file_path);
    
    char* axi_file = get_source(axi_path);


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

void write_arrays_port_map(FILE* new_top_file, bram_interface_t* interface) {
    const char* prefix = "dynamatic_";
    
    fprintf(new_top_file, "\t\t%s", interface->address);
    fprintf(new_top_file, " => %s%s\n", prefix, interface->address);

    fprintf(new_top_file, "\t\t%s", interface->address);
    fprintf(new_top_file, " => %s%s\n", prefix, interface->ce);

    fprintf(new_top_file, "\t\t%s", interface->address);
    fprintf(new_top_file, " => %s%s\n", prefix, interface->we);

    fprintf(new_top_file, "\t\t%s", interface->address);
    fprintf(new_top_file, " => %s%s\n", prefix, interface->dout);

    fprintf(new_top_file, "\t\t%s", interface->address);
    fprintf(new_top_file, " => %s%s\n", prefix, interface->din);
}

void advance_in_file(regex_t* reg, regmatch_t* match, FILE* file, char** offset, const char* pattern) {
    int err = regcomp(reg, pattern, REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(reg, *offset, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }

    fwrite(*offset, sizeof(char), match[0].rm_eo + 1, file);
    *offset += match[0].rm_eo + 1;
}

void write_top_file(project_t* project) {
    regex_t reg;
    regmatch_t match[1];

    FILE* new_top_file = fopen("top_file.tmp", "w");

    char* top_file_off = project->axi_ip.axi_files.top_file;

    advance_in_file(&reg, match, new_top_file, &top_file_off, "-- Users to add ports here");

    hdl_source_t* hdl_source = project->hdl_source;
    
    for(int i = 0; i < hdl_source->nb_arrays; ++i) {
      write_array_ports(new_top_file, &(hdl_source->arrays[i].write_ports));
      write_array_ports(new_top_file, &(hdl_source->arrays[i].read_ports));  
    }

    advance_in_file(&reg, match, new_top_file, &top_file_off, "port [(]");

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

    advance_in_file(&reg, match, new_top_file, &top_file_off, "end component \\w+;");

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

    advance_in_file(&reg, match, new_top_file, &top_file_off, "port map [(]");

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

    advance_in_file(&reg, match, new_top_file, &top_file_off, "-- Add user logic here");

    fprintf(new_top_file, "\t%s_inst : %s\n", hdl_source->name, hdl_source->name);
    fprintf(new_top_file, "\tport map (\n");

    fprintf(new_top_file, "\t\tclk => csr_aclk,\n");
    fprintf(new_top_file, "\t\trst => dynamatic_rst,\n");
    fprintf(new_top_file, "\t\tstart_in => \"0\",\n");
    fprintf(new_top_file, "\t\tstart_valid => dynamatic_start_valid,\n");
    fprintf(new_top_file, "\t\tstart_ready => dynamatic_start_ready,\n");
    fprintf(new_top_file, "\t\tend_out => dynamatic_end_out,\n");
    fprintf(new_top_file, "\t\tend_valid => dynamatic_end_valid,\n");
    fprintf(new_top_file, "\t\tend_ready => dynamatic_end_ready,\n");

    for(int i = 0; i < hdl_source->nb_arrays; ++i) {
        write_arrays_port_map(new_top_file, &(hdl_source->arrays->write_ports));
        write_arrays_port_map(new_top_file, &(hdl_source->arrays->read_ports));
    }

    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\t\t%s_din => dynamatic_%s_din,\n", param_name, param_name);
        fprintf(new_top_file, "\t\t%s_valid_in => '1',\n", param_name);
        if(i == hdl_source->nb_params - 1) {
            fprintf(new_top_file, "\t\t%s_ready_out => OPEN\n", param_name);
        }
        else {
            fprintf(new_top_file, "\t\t%s_ready_out => OPEN,\n", param_name);
        }
    }

    fprintf(new_top_file, "\t);\n");

    advance_in_file(&reg, match, new_top_file, &top_file_off, "end arch_imp;");

    fclose(new_top_file);

    regfree(&reg);
}

void write_axi_file(project_t* project) {
    regex_t reg;
    regmatch_t match[1];

    FILE* new_axi_file = fopen("axi_file.tmp", "w");

    char* axi_file_off = project->axi_ip.axi_files.axi_file;

    hdl_source_t* hdl_source = project->hdl_source;
    advance_in_file(&reg, match, new_axi_file, &axi_file_off, "-- Users to add ports here");

    fprintf(new_axi_file, "\t\taxi_start_valid : out std_logic;\n");
    fprintf(new_axi_file, "\t\taxi_start_ready : in std_logic;\n");
    fprintf(new_axi_file, "\t\taxi_end_out : in std_logic_vector(0 downto 0);\n");
    fprintf(new_axi_file, "\t\taxi_end_valid : in std_logic;\n");
    fprintf(new_axi_file, "\t\taxi_end_ready : out std_logic;\n");

    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_axi_file, "\t\taxi_%s_din : out std_logic_vector(31 downto 0);\n", param_name);
    }

    fprintf(new_axi_file, "\t\taxi_reset : out std_logic;\n");

    advance_in_file(&reg, match, new_axi_file, &axi_file_off, "process [(]slv_reg0, ");
    //We skip slv_reg1
    axi_file_off += strlen("slv_reg1, ");
    fprintf(new_axi_file, "axi_start_ready, axi_end_out, axi_end_valid, ");

    advance_in_file(&reg, match, new_axi_file, &axi_file_off, "reg_data_out <= slv_reg1;");
    
    //We want to erase slv_reg1;
    fseek(new_axi_file, -9, SEEK_CUR);
    fprintf(new_axi_file, "(31 downto 3 => '0') & axi_start_ready & axi_end_out & axi_end_valid;");

    advance_in_file(&reg, match, new_axi_file, &axi_file_off, "-- Add user logic here");

    fprintf(new_axi_file, "\taxi_start_valid <= slv_reg0(0);\n");
    fprintf(new_axi_file, "\taxi_end_ready <= slv_reg0(1);\n");
    fprintf(new_axi_file, "\taxi_reset <= slv_reg0(2);\n");
    int slv_reg_nb = 2;
    for(int i = 0; i < hdl_source->nb_params; ++i) {
        const char* param_name = hdl_source->params[i].name;
        fprintf(new_axi_file, "\taxi_%s_din <= slv_reg%d;\n", param_name, slv_reg_nb);
        slv_reg_nb++;
    }

    advance_in_file(&reg, match, new_axi_file, &axi_file_off, "end arch_imp;");
    fclose(new_axi_file);
    regfree(&reg);
}

void update_top_file(project_t* project) {
    write_top_file(project);
    char* new_top_file_src = get_source("top_file.tmp");
    remove("top_file.tmp");

    //Open real file and replace its content
    FILE* top_file = fopen(project->axi_ip.axi_files.top_file_path, "w");
    fwrite(new_top_file_src, sizeof(char), strlen(new_top_file_src), top_file);
    fclose(top_file);
}

void update_axi_file(project_t* project) {
    write_axi_file(project);
    char* new_axi_file_src = get_source("axi_file.tmp");
    remove("axi_file.tmp");

    //Open real file and replace its content
    char axi_path[MAX_NAME_LENGTH];
    char axi_path_suffix[MAX_NAME_LENGTH];
    strcpy(axi_path, project->axi_ip.path);
    sprintf(axi_path_suffix, "/%s_1.0/hdl/%s_v1.0.vhd", project->axi_ip.name,
     project->axi_ip.name);
    strcpy(axi_path + strlen(project->axi_ip.path), axi_path_suffix);
    FILE* axi_file = fopen(axi_path, "w");
    fwrite(new_axi_file_src, sizeof(char), strlen(new_axi_file_src), axi_file);
    fclose(axi_file);
}

void update_files(project_t* project) {
    update_top_file(project);
    update_axi_file(project);

}
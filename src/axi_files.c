#include "axi_files.h"
#include <inttypes.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

auto_error_t create_axi(axi_ip_t *axi_ip, project_t *project) {
    CHECK_PARAM(axi_ip);
    CHECK_PARAM(project);

    memset(axi_ip, 0, sizeof(axi_ip_t));

    // Added end_out
    axi_ip->nb_slave_registers = project->hdl_source->nb_params + 3;

    // Minimum is four slave registers
    if (axi_ip->nb_slave_registers < 4) {
        axi_ip->nb_slave_registers = 4;
    }

    axi_ip->data_width = project->hdl_source->max_param_width;
    axi_ip->addr_width = (uint16_t)(
        (uint16_t)(63 - __builtin_clzll(axi_ip->nb_slave_registers - 1) + 1) +
        (uint16_t)(63 - __builtin_clzll(axi_ip->data_width >> 3)));

    strncpy(axi_ip->path, project->path, MAX_PATH_LENGTH);
    CHECK_LENGTH(strlen(axi_ip->path) + strlen("/ip_repo") + 1,
                 MAX_PATH_LENGTH);

    CHECK_COND((size_t)(MAX_PATH_LENGTH - strlen(axi_ip->path)) >
                       (size_t)MAX_PATH_LENGTH ||
                   MAX_PATH_LENGTH - strlen(axi_ip->path) < strlen("/ip_repo"),
               ERR_NAME_TOO_LONG, "Name too long !");
    strncpy(axi_ip->path + strlen(axi_ip->path), "/ip_repo",
            MAX_PATH_LENGTH - strlen("/ip_repo"));
    axi_ip->revision = 1;

    int written = snprintf(axi_ip->name, MAX_NAME_LENGTH - strlen("axi_ip_"),
                           "axi_ip_%s", project->hdl_source->name);
    CHECK_LENGTH(written, (int)(MAX_NAME_LENGTH - strlen("axi_ip_")));
    return ERR_NONE;
}

char *read_hdl_file(axi_ip_t *axi_ip, char *set_file_path,
                    const char *file_name) {
    if (axi_ip == NULL || set_file_path == NULL || file_name == NULL) {
        return NULL;
    }

    char file_path[MAX_PATH_LENGTH];
    memset(file_path, 0, MAX_PATH_LENGTH * sizeof(char));

    char file_path_suffix[MAX_NAME_LENGTH];
    memset(file_path_suffix, 0, MAX_NAME_LENGTH * sizeof(char));

    strncpy(file_path, axi_ip->path, MAX_PATH_LENGTH);

    int written = snprintf(file_path_suffix, MAX_NAME_LENGTH, "/%s_1.0/hdl/%s",
                           axi_ip->name, file_name);
    if (written >= MAX_NAME_LENGTH) {
        return NULL;
    }
    if (strlen(axi_ip->path) + strlen(file_path_suffix) + 1 >=
        MAX_PATH_LENGTH) {
        return NULL;
    }
    if ((size_t)(MAX_PATH_LENGTH - strlen(axi_ip->path)) >
            (size_t)MAX_PATH_LENGTH ||
        MAX_PATH_LENGTH - strlen(axi_ip->path) < strlen(file_path_suffix)) {
        return NULL;
    }
    strncpy(file_path + strlen(axi_ip->path), file_path_suffix,
            MAX_PATH_LENGTH - strlen(axi_ip->path));

    strncpy(set_file_path, file_path, MAX_PATH_LENGTH);

    char *top_file = read_file(file_path, NULL);

    return top_file;
}

auto_error_t read_axi_files(axi_ip_t *axi_ip) {
    CHECK_PARAM(axi_ip);

    char top_file_name[MAX_NAME_LENGTH];
    memset(top_file_name, 0, MAX_NAME_LENGTH * sizeof(char));

    int written =
        snprintf(top_file_name, MAX_NAME_LENGTH, "%s_v1_0.vhd", axi_ip->name);
    CHECK_LENGTH(written, MAX_NAME_LENGTH);
    char *top_file =
        read_hdl_file(axi_ip, axi_ip->axi_files.top_file_path, top_file_name);
    CHECK_NULL(top_file, ERR_IO,
               "Didn't manage to read the top HDL file of the axi IP");

    char axi_file_name[MAX_NAME_LENGTH];
    memset(axi_file_name, 0, MAX_NAME_LENGTH * sizeof(char));

    written = snprintf(axi_file_name, MAX_NAME_LENGTH, "%s_v1_0_%s.vhd",
                       axi_ip->name, axi_ip->interface_name);
    CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                  "Name too long !", free((void *)top_file););
    char *axi_file =
        read_hdl_file(axi_ip, axi_ip->axi_files.axi_file_path, axi_file_name);
    CHECK_COND_DO(axi_file == NULL, ERR_IO,
                  "Didn't manage to read the top HDL file of the axi IP",
                  free((void *)top_file););

    axi_ip->axi_files.top_file = top_file;
    axi_ip->axi_files.axi_file = axi_file;

    return ERR_NONE;
}

auto_error_t write_array_ports(FILE *file, bram_interface_t *interface,
                               size_t array_width) {
    CHECK_PARAM(file);
    CHECK_PARAM(interface);

    const char *prefix = "dynamatic_";

    fprintf(file, "\t\t%s", prefix);
    fprintf(file, "%s", interface->address);
    fprintf(file, "%s\n", " : out std_logic_vector (31 downto 0);");

    fprintf(file, "\t\t%s", prefix);
    fprintf(file, "%s", interface->ce);
    fprintf(file, "%s\n", " : out std_logic;");

    fprintf(file, "\t\t%s", prefix);
    fprintf(file, "%s", interface->we);
    fprintf(file, "%s\n", " : out std_logic;");

    fprintf(file, "\t\t%s", prefix);
    fprintf(file, "%s", interface->dout);
    fprintf(file, " : out std_logic_vector (%zu downto 0);\n", array_width - 1);

    fprintf(file, "\t\t%s", prefix);
    fprintf(file, "%s", interface->din);
    fprintf(file, " : in std_logic_vector (%zu downto 0);\n", array_width - 1);

    return ERR_NONE;
}

auto_error_t write_array_ports_wo_prefix(FILE *file,
                                         bram_interface_t *interface, bool last,
                                         size_t array_width) {
    CHECK_PARAM(file);
    CHECK_PARAM(interface);

    fprintf(file, "\t\t%s", interface->address);
    fprintf(file, "%s\n", " : out std_logic_vector (31 downto 0);");

    fprintf(file, "\t\t%s", interface->ce);
    fprintf(file, "%s\n", " : out std_logic;");

    fprintf(file, "\t\t%s", interface->we);
    fprintf(file, "%s\n", " : out std_logic;");

    fprintf(file, "\t\t%s", interface->dout);
    fprintf(file, " : out std_logic_vector (%zu downto 0);\n", array_width - 1);
    if (last) {
        fprintf(file, "\t\t%s", interface->din);
        fprintf(file, " : in std_logic_vector (%zu downto 0)\n",
                array_width - 1);
    } else {
        fprintf(file, "\t\t%s", interface->din);
        fprintf(file, " : in std_logic_vector (%zu downto 0);\n",
                array_width - 1);
    }

    return ERR_NONE;
}

auto_error_t write_arrays_port_map(FILE *file, bram_interface_t *interface,
                                   bool last) {
    CHECK_PARAM(file);
    CHECK_PARAM(interface);

    const char *prefix = "dynamatic_";

    fprintf(file, "\t\t%s", interface->address);
    fprintf(file, " => %s%s,\n", prefix, interface->address);

    fprintf(file, "\t\t%s", interface->ce);
    fprintf(file, " => %s%s,\n", prefix, interface->ce);

    fprintf(file, "\t\t%s", interface->we);
    fprintf(file, " => %s%s,\n", prefix, interface->we);

    fprintf(file, "\t\t%s", interface->dout);
    fprintf(file, " => %s%s,\n", prefix, interface->dout);

    if (last) {
        fprintf(file, "\t\t%s", interface->din);
        fprintf(file, " => %s%s\n", prefix, interface->din);
    } else {
        fprintf(file, "\t\t%s", interface->din);
        fprintf(file, " => %s%s,\n", prefix, interface->din);
    }

    return ERR_NONE;
}

auto_error_t advance_in_file(regex_t *reg, regmatch_t *match, FILE *file,
                             const char **offset, const char *pattern) {
    CHECK_PARAM(reg);
    CHECK_PARAM(match);
    CHECK_PARAM(file);
    CHECK_PARAM(offset);
    CHECK_PARAM(*offset);
    CHECK_PARAM(pattern);

    int err = regcomp(reg, pattern, REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    err = regexec(reg, *offset, 1, (regmatch_t *)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(reg););

    regfree(reg);

    fwrite(*offset, sizeof(char), (size_t)(match[0].rm_eo + 1), file);
    *offset += match[0].rm_eo + 1;

    return ERR_NONE;
}

auto_error_t write_top_file(project_t *project, axi_ip_t *axi_ip) {
    CHECK_PARAM(project);
    CHECK_PARAM(project->hdl_source);
    CHECK_PARAM(axi_ip->axi_files.top_file)

    regex_t reg;
    regmatch_t match[1];

    FILE *new_top_file = fopen("top_file.tmp", "w");
    CHECK_NULL(new_top_file, ERR_IO, "Could not open file : top_file.tmp");

    const char *top_file_off = axi_ip->axi_files.top_file;

    hdl_source_t *hdl_source = project->hdl_source;
    CHECK_COND_DO(hdl_source == NULL, ERR_NULL, "hdl_source is NULL !",
                  fclose(new_top_file););

    char pattern[MAX_NAME_LENGTH];
    int written = snprintf(pattern, MAX_NAME_LENGTH,
                           "C_%s_DATA_WIDTH\\s+\\:\\s+integer\\s+:=\\s+32;",
                           axi_ip->interface_name);
    CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                  "Name too long !", fclose(new_top_file););

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_top_file, &top_file_off, pattern),
        "advance in file failed !", fclose(new_top_file););

    fseek(new_top_file, -4, SEEK_CUR);

    fprintf(new_top_file, "%" PRIu16 ";\n", axi_ip->data_width);

    written = snprintf(pattern, MAX_NAME_LENGTH,
                       "C_%s_ADDR_WIDTH\\s+\\:\\s+integer\\s+:=\\s+4",
                       axi_ip->interface_name);
    CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                  "Name too long !", fclose(new_top_file););

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_top_file, &top_file_off, pattern),
        "advance in file failed !", fclose(new_top_file););

    fseek(new_top_file, -2, SEEK_CUR);

    fprintf(new_top_file, "%" PRIu16 "\n", axi_ip->addr_width);

    CHECK_CALL_DO(advance_in_file(&reg, match, new_top_file, &top_file_off,
                                  "-- Users to add ports here"),
                  "advance in file failed !", fclose(new_top_file););

    for (size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        CHECK_CALL_DO(write_array_ports(new_top_file,
                                        &(hdl_source->arrays[i].write_ports),
                                        hdl_source->arrays[i].width),
                      "write_array_ports failed !", fclose(new_top_file));
        CHECK_CALL_DO(write_array_ports(new_top_file,
                                        &(hdl_source->arrays[i].read_ports),
                                        hdl_source->arrays[i].width),
                      "write_array_ports failed !", fclose(new_top_file));
    }

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_top_file, &top_file_off,
                        "C_S_AXI_DATA_WIDTH\\s+\\:\\s+integer\\s+:=\\s+32;"),
        "advance in file failed !", fclose(new_top_file););

    fseek(new_top_file, -4, SEEK_CUR);
    fprintf(new_top_file, "%" PRIu16 ";\n", axi_ip->data_width);

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_top_file, &top_file_off,
                        "C_S_AXI_ADDR_WIDTH\\s+\\:\\s+integer\\s+:=\\s+4"),
        "advance in file failed !", fclose(new_top_file););

    fseek(new_top_file, -2, SEEK_CUR);

    fprintf(new_top_file, "%" PRIu16 "\n", axi_ip->addr_width);

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_top_file, &top_file_off, "port [(]"),
        "advance in file failed !", fclose(new_top_file););

    fprintf(new_top_file, "\n");
    fprintf(new_top_file, "\t\taxi_start_valid : out std_logic;\n");
    fprintf(new_top_file, "\t\taxi_start_ready : in std_logic;\n");
    fprintf(new_top_file,
            "\t\taxi_end_out : in std_logic_vector(%" PRIu16 " downto 0);\n",
            hdl_source->end_out_width - 1);
    fprintf(new_top_file, "\t\taxi_end_valid : in std_logic;\n");
    fprintf(new_top_file, "\t\taxi_end_ready : out std_logic;\n");

    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_top_file,
                "\t\taxi_%s_din : out std_logic_vector(%" PRIu16
                " downto 0);\n",
                param_name, hdl_source->params[i].width - 1);
    }

    fprintf(new_top_file, "\t\taxi_reset : out std_logic;\n");

    CHECK_CALL_DO(advance_in_file(&reg, match, new_top_file, &top_file_off,
                                  "end component \\w+;"),
                  "advance in file failed !", fclose(new_top_file););

    fprintf(new_top_file, "\n");

    fprintf(new_top_file, "\tcomponent %s is\n", hdl_source->name);
    fprintf(new_top_file, "\t\tport (\n");

    fprintf(new_top_file, "\t\tclk:  in std_logic;\n");
    fprintf(new_top_file, "\t\trst:  in std_logic;\n");
    fprintf(new_top_file, "\t\tstart_in:  in std_logic_vector (0 downto 0);\n");
    fprintf(new_top_file, "\t\tstart_valid:  in std_logic;\n");
    fprintf(new_top_file, "\t\tstart_ready:  out std_logic;\n");
    fprintf(new_top_file,
            "\t\tend_out:  out std_logic_vector (%" PRIu16 " downto 0);\n",
            hdl_source->end_out_width - 1);
    fprintf(new_top_file, "\t\tend_valid:  out std_logic;\n");
    if (hdl_source->nb_arrays == 0 && hdl_source->nb_params == 0) {
        fprintf(new_top_file, "\t\tend_ready:  in std_logic\n");
    } else {
        fprintf(new_top_file, "\t\tend_ready:  in std_logic;\n");
    }

    bool last;

    for (size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        last = (i == hdl_source->nb_arrays - 1 && hdl_source->nb_params == 0);
        CHECK_CALL_DO(write_array_ports_wo_prefix(
                          new_top_file, &(hdl_source->arrays[i].write_ports),
                          false, hdl_source->arrays[i].width),
                      "write_array_ports_wo_prefix failed !",
                      fclose(new_top_file));
        CHECK_CALL_DO(write_array_ports_wo_prefix(
                          new_top_file, &(hdl_source->arrays[i].read_ports),
                          last, hdl_source->arrays[i].width),
                      "write_array_ports_wo_prefix failed !",
                      fclose(new_top_file));
    }

    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_top_file,
                "\t\t%s_din : in std_logic_vector (%" PRIu16 " downto 0);\n",
                param_name, hdl_source->params[i].width - 1);
        fprintf(new_top_file, "\t\t%s_valid_in : in std_logic;\n", param_name);
        if (i == hdl_source->nb_params - 1) {
            fprintf(new_top_file, "\t\t%s_ready_out : out std_logic\n",
                    param_name);
        } else {
            fprintf(new_top_file, "\t\t%s_ready_out : out std_logic;\n",
                    param_name);
        }
    }

    fprintf(new_top_file, "\t\t);\n");
    fprintf(new_top_file, "\tend component %s;\n", hdl_source->name);

    fprintf(new_top_file, "\n");

    fprintf(new_top_file, "\tsignal dynamatic_rst : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_start_valid : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_start_ready : std_logic;\n");
    fprintf(new_top_file,
            "\tsignal dynamatic_end_out : std_logic_vector(%" PRIu16
            " downto 0);\n",
            hdl_source->end_out_width - 1);
    fprintf(new_top_file, "\tsignal dynamatic_end_valid : std_logic;\n");
    fprintf(new_top_file, "\tsignal dynamatic_end_ready : std_logic;\n");

    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_top_file,
                "\tsignal dynamatic_%s_din : std_logic_vector(%" PRIu16
                " downto 0);\n",
                param_name, hdl_source->params[i].width - 1);
    }

    CHECK_CALL_DO(advance_in_file(&reg, match, new_top_file, &top_file_off,
                                  "port map [(]"),
                  "advance in file failed !", fclose(new_top_file););

    fprintf(new_top_file, "\t\taxi_start_valid => dynamatic_start_valid,\n");
    fprintf(new_top_file, "\t\taxi_start_ready => dynamatic_start_ready,\n");
    fprintf(new_top_file, "\t\taxi_end_out => dynamatic_end_out,\n");
    fprintf(new_top_file, "\t\taxi_end_valid => dynamatic_end_valid,\n");
    fprintf(new_top_file, "\t\taxi_end_ready => dynamatic_end_ready,\n");

    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\t\taxi_%s_din => dynamatic_%s_din,\n",
                param_name, param_name);
    }

    fprintf(new_top_file, "\t\taxi_reset => dynamatic_rst,\n");

    CHECK_CALL_DO(advance_in_file(&reg, match, new_top_file, &top_file_off,
                                  "-- Add user logic here"),
                  "advance in file failed !", fclose(new_top_file););

    fprintf(new_top_file, "\t%s_inst : %s\n", hdl_source->name,
            hdl_source->name);
    fprintf(new_top_file, "\tport map (\n");

    fprintf(new_top_file, "\t\tclk => csr_aclk,\n");
    fprintf(new_top_file, "\t\trst => dynamatic_rst,\n");
    fprintf(new_top_file, "\t\tstart_in => \"0\",\n");
    fprintf(new_top_file, "\t\tstart_valid => dynamatic_start_valid,\n");
    fprintf(new_top_file, "\t\tstart_ready => dynamatic_start_ready,\n");
    fprintf(new_top_file, "\t\tend_out => dynamatic_end_out,\n");
    fprintf(new_top_file, "\t\tend_valid => dynamatic_end_valid,\n");
    if (hdl_source->nb_arrays == 0 && hdl_source->nb_params == 0) {
        fprintf(new_top_file, "\t\tend_ready => dynamatic_end_ready\n");
    } else {
        fprintf(new_top_file, "\t\tend_ready => dynamatic_end_ready,\n");
    }

    for (size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        last = (i == hdl_source->nb_arrays - 1 && hdl_source->nb_params == 0);
        CHECK_CALL_DO(
            write_arrays_port_map(new_top_file,
                                  &(hdl_source->arrays[i].write_ports), false),
            "write_arrays_port_map failed !", fclose(new_top_file));
        CHECK_CALL_DO(write_arrays_port_map(new_top_file,
                                            &(hdl_source->arrays[i].read_ports),
                                            last),
                      "write_arrays_port_map failed !", fclose(new_top_file));
    }

    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_top_file, "\t\t%s_din => dynamatic_%s_din,\n", param_name,
                param_name);
        fprintf(new_top_file, "\t\t%s_valid_in => '1',\n", param_name);
        if (i == hdl_source->nb_params - 1) {
            fprintf(new_top_file, "\t\t%s_ready_out => OPEN\n", param_name);
        } else {
            fprintf(new_top_file, "\t\t%s_ready_out => OPEN,\n", param_name);
        }
    }

    fprintf(new_top_file, "\t);\n");

    const char *cur = top_file_off;
    const char *top_file = axi_ip->axi_files.top_file;
    const char *end = top_file + strlen(top_file);
    size_t remaining = (size_t)(end - cur);
    fwrite(top_file_off, sizeof(char), remaining, new_top_file);

    fclose(new_top_file);

    regfree(&reg);

    return ERR_NONE;
}

auto_error_t write_axi_file(project_t *project, axi_ip_t *axi_ip) {
    CHECK_PARAM(project);
    CHECK_PARAM(project->hdl_source);
    CHECK_PARAM(axi_ip->axi_files.axi_file);

    regex_t reg;
    regmatch_t match[1];

    FILE *new_axi_file = fopen("axi_file.tmp", "w");
    CHECK_NULL(new_axi_file, ERR_IO, "Could not open file : axi_file.tmp");

    const char *axi_file_off = axi_ip->axi_files.axi_file;

    hdl_source_t *hdl_source = project->hdl_source;

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_axi_file, &axi_file_off,
                        "C_S_AXI_DATA_WIDTH\\s+\\:\\s+integer\\s+:=\\s+32;"),
        "advance in file failed !", fclose(new_axi_file););

    fseek(new_axi_file, -4, SEEK_CUR);

    fprintf(new_axi_file, "%" PRIu16 ";\n", axi_ip->data_width);

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_axi_file, &axi_file_off,
                        "C_S_AXI_ADDR_WIDTH\\s+\\:\\s+integer\\s+:=\\s+4"),
        "advance in file failed !", fclose(new_axi_file););

    fseek(new_axi_file, -2, SEEK_CUR);
    fprintf(new_axi_file, "%" PRIu16 "\n", axi_ip->addr_width);

    CHECK_CALL_DO(advance_in_file(&reg, match, new_axi_file, &axi_file_off,
                                  "-- Users to add ports here"),
                  "advance in file failed !", fclose(new_axi_file););

    fprintf(new_axi_file, "\t\taxi_start_valid : out std_logic;\n");
    fprintf(new_axi_file, "\t\taxi_start_ready : in std_logic;\n");
    fprintf(new_axi_file,
            "\t\taxi_end_out : in std_logic_vector(%" PRIu16 " downto 0);\n",
            hdl_source->end_out_width - 1);
    fprintf(new_axi_file, "\t\taxi_end_valid : in std_logic;\n");
    fprintf(new_axi_file, "\t\taxi_end_ready : out std_logic;\n");

    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_axi_file,
                "\t\taxi_%s_din : out std_logic_vector(%" PRIu16
                " downto 0);\n",
                param_name, hdl_source->params[i].width - 1);
    }

    fprintf(new_axi_file, "\t\taxi_reset : out std_logic;\n");

    CHECK_CALL_DO(advance_in_file(&reg, match, new_axi_file, &axi_file_off,
                                  "process [(]slv_reg0,"),
                  "advance in file failed !", fclose(new_axi_file););
    // We skip slv_reg1
    axi_file_off += strlen("slv_reg1, ");
    fprintf(new_axi_file, "axi_start_ready, axi_end_out, axi_end_valid, ");

    CHECK_CALL_DO(advance_in_file(&reg, match, new_axi_file, &axi_file_off,
                                  "reg_data_out <= slv_reg1;"),
                  "advance in file failed !", fclose(new_axi_file););

    // We want to erase slv_reg1;
    fseek(new_axi_file, -10, SEEK_CUR);
    fprintf(new_axi_file,
            "(%" PRIu16
            " downto 2 => '0') & axi_start_ready & axi_end_valid;\n",
            axi_ip->data_width - 1);

    size_t last_reg = axi_ip->nb_slave_registers - 1;
    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, MAX_NAME_LENGTH * sizeof(char));

    int written = snprintf(pattern, MAX_NAME_LENGTH,
                           "reg_data_out <= slv_reg%zu", last_reg);
    CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                  "Name too long !", fclose(new_axi_file););

    CHECK_CALL_DO(
        advance_in_file(&reg, match, new_axi_file, &axi_file_off, pattern),
        "advance in file failed !", fclose(new_axi_file););

    int16_t nb_removed = 0;
    if (last_reg < 10) {
        nb_removed = 13;
    } else if (last_reg >= 10 && last_reg < 100) {
        nb_removed = 14;
    } else {
        nb_removed = 15;
    }
    fseek(new_axi_file, -nb_removed, SEEK_CUR);
    fprintf(new_axi_file, "(%" PRIu16 " downto 0) <= axi_end_out;",
            hdl_source->end_out_width - 1);

    CHECK_CALL_DO(advance_in_file(&reg, match, new_axi_file, &axi_file_off,
                                  "-- Add user logic here"),
                  "advance in file failed !", fclose(new_axi_file););

    fprintf(new_axi_file, "\taxi_start_valid <= slv_reg0(0);\n");
    fprintf(new_axi_file, "\taxi_end_ready <= slv_reg0(1);\n");
    fprintf(new_axi_file, "\taxi_reset <= slv_reg0(2);\n");
    int slv_reg_nb = 2;
    for (size_t i = 0; i < hdl_source->nb_params; ++i) {
        const char *param_name = hdl_source->params[i].name;
        fprintf(new_axi_file,
                "\taxi_%s_din <= slv_reg%d(%" PRIu16 " downto 0);\n",
                param_name, slv_reg_nb, hdl_source->params[i].width - 1);
        slv_reg_nb++;
    }

    const char *cur = axi_file_off;
    const char *axi_file = axi_ip->axi_files.axi_file;
    const char *end = axi_file + strlen(axi_file);
    size_t remaining = (size_t)(end - cur);
    fwrite(axi_file_off, sizeof(char), remaining, new_axi_file);
    fclose(new_axi_file);
    regfree(&reg);

    return ERR_NONE;
}

auto_error_t update_top_file(project_t *project, axi_ip_t *axi_ip) {
    CHECK_PARAM(project);
    CHECK_PARAM(axi_ip->axi_files.top_file_path);

    CHECK_CALL(write_top_file(project, axi_ip), "write_top_file failed !");
    size_t size;
    char *new_top_file_src = read_file("top_file.tmp", &size);
    CHECK_COND(new_top_file_src == NULL, ERR_IO, "get_source failed !");
    remove("top_file.tmp");

    // Open real file and replace its content
    FILE *top_file = fopen(axi_ip->axi_files.top_file_path, "w");
    CHECK_COND_DO(top_file == NULL, ERR_IO,
                  "Could not open file : top hdl file",
                  free((void *)new_top_file_src);
                  fclose(top_file););

    fwrite(new_top_file_src, sizeof(char), size, top_file);
    fclose(top_file);
    free((void *)new_top_file_src);

    return ERR_NONE;
}

auto_error_t update_axi_file(project_t *project, axi_ip_t *axi_ip) {
    CHECK_PARAM(project);
    CHECK_PARAM(axi_ip->axi_files.axi_file_path);

    CHECK_CALL(write_axi_file(project, axi_ip), "write_axi_file failed !");

    size_t size;
    char *new_axi_file_src = read_file("axi_file.tmp", &size);
    CHECK_COND(new_axi_file_src == NULL, ERR_IO, "get_source failed !");
    remove("axi_file.tmp");

    // Open real file and replace its content
    FILE *axi_file = fopen(axi_ip->axi_files.axi_file_path, "w");
    CHECK_COND_DO(axi_file == NULL, ERR_IO,
                  "Could not open file : axi hdl file",
                  free((void *)new_axi_file_src);
                  fclose(axi_file););
    fwrite(new_axi_file_src, sizeof(char), size, axi_file);
    fclose(axi_file);
    free((void *)new_axi_file_src);

    return ERR_NONE;
}

auto_error_t update_files(project_t *project, axi_ip_t *axi_ip) {
    CHECK_CALL(update_top_file(project, axi_ip), "update_top_file failed !");
    CHECK_CALL(update_axi_file(project, axi_ip), "update_axi_file failed !");
    return ERR_NONE;
}

auto_error_t free_axi(axi_ip_t *axi_ip) {

    if (axi_ip->axi_files.top_file != NULL) {
        free((void *)(axi_ip->axi_files.top_file));
        axi_ip->axi_files.top_file = NULL;
    }
    if (axi_ip->axi_files.axi_file != NULL) {
        free((void *)(axi_ip->axi_files.axi_file));
        axi_ip->axi_files.axi_file = NULL;
    }

    return ERR_NONE;
}
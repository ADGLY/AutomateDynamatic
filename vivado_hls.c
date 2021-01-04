#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>
#include <dirent.h>
#include "vivado_hls.h"

auto_error_t create_hls(vivado_hls_t* hls, hdl_source_t* hdl_source) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hdl_source);

    memset(hls, 0, sizeof(vivado_hls_t));

    char path_temp[MAX_PATH_LENGTH];
    strncpy(path_temp, hdl_source->dir, MAX_PATH_LENGTH);
    uint16_t max = (uint16_t)(MAX_PATH_LENGTH - strlen(path_temp) - 1);

    char* temp_hdl_name = strrchr(hdl_source->top_file_path, '/') + 1;
    char pattern[MAX_NAME_LENGTH];

    regex_t reg;
    regmatch_t match[2];

    char hdl_name[MAX_NAME_LENGTH];
    strncpy(hdl_name, temp_hdl_name, MAX_NAME_LENGTH);
    *strrchr(hdl_name, '.') = '\0';
    int written = snprintf(pattern, MAX_NAME_LENGTH, "(.*)_optimized");
    CHECK_LENGTH(written, MAX_NAME_LENGTH);

    int err = regcomp(&reg, pattern, REG_EXTENDED);
    err = regexec(&reg, hdl_name, 2, match, 0);

    char cpp_name[MAX_NAME_LENGTH];
    memset(cpp_name, 0, MAX_NAME_LENGTH * sizeof(char));

    if(err == REG_NOMATCH) {
        strncpy(cpp_name, hdl_name, MAX_NAME_LENGTH);
    }
    else {
        strncpy(cpp_name, hdl_name, (size_t)match[1].rm_eo);
    }
    regfree(&reg);
    
    written = snprintf(path_temp + strlen(path_temp), max, "/../src/%s.cpp", cpp_name);
    CHECK_LENGTH(written, max);

    char* final_source_path = realpath(path_temp, hls->source_path);
    CHECK_COND(final_source_path == NULL && errno != ENOENT, ERR_PATH, "Could not resolve absolute path !");

    char* final_path = realpath("vivado_hls", hls->project_path);
    CHECK_COND(final_path == NULL && errno != ENOENT, ERR_PATH, "Could not resolve absolute path !");

    char* source_file = get_source(hls->source_path, NULL);
    CHECK_NULL(source_file, ERR_FILE, "Could not read the C/C++ source file !");
    hls->hls_source = source_file;
    return ERR_NONE;
}

auto_error_t advance_in_file_hls(regex_t* reg, regmatch_t* match, const char** source_off, const char* pattern, const char** str_match,
    size_t* match_len) {
    int err = regcomp(reg, pattern, REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    err = regexec(reg, *source_off, 1, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(reg););

    regfree(reg);

    *str_match = *source_off + match[0].rm_so;
    *match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    *source_off = (const char*)(*source_off + match[0].rm_so + *match_len);

    return ERR_NONE;
}

auto_error_t parse_hls(vivado_hls_t* hls, hdl_source_t* hdl_source) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hls->hls_source);
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->arrays);

    regex_t reg;
    regmatch_t match[3];

    const char* str_match;
    size_t str_match_len;

    const char* source_off = hls->hls_source;

    char* new_source = realloc(hls->hls_source, strlen(hls->hls_source) + 1);
    CHECK_NULL(new_source, ERR_MEM, "Realloc failed on hls source !");

    hls->hls_source = new_source;

    source_off = hls->hls_source;
    
    int err = regcomp(&reg, "\\w+\\s+(\\w+)\\s*(\\()[^(\\))]*\\)[[:space:]]*\\{", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    err = regexec(&reg, source_off, 3, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););

    regfree(&reg);

    size_t sub_match_len = (size_t)(match[1].rm_eo - match[1].rm_so);
    CHECK_COND_DO(sub_match_len >= MAX_NAME_LENGTH, ERR_REGEX, "Name too long !", regfree(&reg););
    strncpy(hls->fun_name, source_off + match[1].rm_so, sub_match_len);
    hls->fun_name[sub_match_len] = '\0';

    size_t end_of_prototype = (size_t)(match[0].rm_eo - match[2].rm_eo);

    str_match = source_off + match[0].rm_so;
    str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    source_off = (const char*)(source_off + match[2].rm_eo);
   
    err = regcomp(&reg, "\\w+\\s+(\\w+)\\s*(\\()[^(\\))]*\\)[[:space:]]*\\{", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    err = regexec(&reg, source_off, 3, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););

    regfree(&reg);

    char save = source_off[match[0].rm_so - 1];
    char* modif = &source_off[match[0].rm_so - 1];
    *modif = '\0';



    for(size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        const char* name = hdl_source->arrays[i].name;
        char pattern[MAX_NAME_LENGTH];
        snprintf(pattern, MAX_NAME_LENGTH, "\\w+(\\s)*%s(\\s)*\\[", name);

        err = regcomp(&reg, pattern, REG_EXTENDED);
        CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

        err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
        CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););
        regfree(&reg);
        //Still works if the usal types aren't used
    
        str_match = source_off + match[0].rm_so;
        str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);

        hdl_source->arrays[i].read = false;
        hdl_source->arrays[i].write = false;

        if(strncmp("in_", str_match, 3) == 0) {
            hdl_source->arrays[i].read = true;
        }
        else if(strncmp("inout_", str_match, 6) == 0) {
            hdl_source->arrays[i].read = true;
            hdl_source->arrays[i].write = true;
        }
        else if(strncmp("out_", str_match, 4) == 0) {
            hdl_source->arrays[i].write = true;
        }
        else {
            //Read matching
            const char* look_for_arrays = source_off + end_of_prototype;

            snprintf(pattern, MAX_NAME_LENGTH, "%s[[:space:]]*\\[[^\\]]*\\][^;=]*;", name);
            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");


            err = regexec(&reg, look_for_arrays, 1, (regmatch_t*)match, 0);
            CHECK_COND_DO(err != 0 && err != REG_NOMATCH, ERR_REGEX, "Failed to macth !", regfree(&reg););
            regfree(&reg);
            uint8_t nb_no_match = 0;
            if(err == 0) {
                hdl_source->arrays[i].read = true;
            }
            else {
                //TODO: Detection for things like this *array or *(array + i * sizeof(char)) = value; --> Be careful about the fact that there can be ) in the expression
                /*snprintf(pattern, MAX_NAME_LENGTH, "(\\*%s|        \\*\\(          )[^;=]*;", name);

                err = regcomp(&reg, pattern, REG_EXTENDED);
                CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

                err = regexec(&reg, look_for_arrays, 1, (regmatch_t*)match, 0);
                CHECK_COND_DO(err != 0 && err != REG_NOMATCH, ERR_REGEX, "Failed to macth !", regfree(&reg););
                regfree(&reg);*/
                nb_no_match++;
            }


            //Write matching

            snprintf(pattern, MAX_NAME_LENGTH, "%s[[:space:]]*\\[[^\\]]*\\][^=;]*=[^;]*;", name);
            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

            err = regexec(&reg, look_for_arrays, 1, (regmatch_t*)match, 0);
            CHECK_COND_DO(err != 0 && err != REG_NOMATCH, ERR_REGEX, "Failed to macth !", regfree(&reg););
            regfree(&reg);
            if(err == 0) {
                hdl_source->arrays[i].write = true;
            }
            else {
                nb_no_match++;
            }
            if(nb_no_match == 2) {
                CHECK_CALL(ERR_REGEX, "Error while trying to match on arrays !");
                hdl_source->arrays[i].read = true;
                hdl_source->arrays[i].write = true;
            }
        }

    }
    *modif = save;
    return ERR_NONE;
}

auto_error_t launch_hls_script() {

    FILE* vivado_hls_input;
    vivado_hls_input = popen("vivado_hls -f hls_script.tcl", "w");
    CHECK_COND(vivado_hls_input == NULL, ERR_FILE, "Failed to launch Vivado !");
    fprintf(vivado_hls_input, "exit\n");
    pclose(vivado_hls_input);
    return ERR_NONE;
}

typedef enum {
    HDL, TCL
} file_type_t;

float_op_t* check_op_already_exists(uint8_t count, const char* op_name, float_op_t* float_op) {
    for(uint8_t i = 0; i < count; ++i) {
        float_op_t* op = &(float_op[i]);
        if(strcmp(op_name, op->name) == 0) {
            return op;
        }
    }
    return NULL;
}

auto_error_t check_file(uint8_t* count, regex_t* reg, regmatch_t* match, float_op_t* floats, const char* float_paths, mode_t mode, uint8_t match_idx, 
    struct dirent* dir) {
    
    CHECK_PARAM(count);
    CHECK_PARAM(reg);
    CHECK_PARAM(match);
    CHECK_PARAM(floats);
    CHECK_PARAM(float_paths);
    CHECK_PARAM(dir);

    int err = regexec(reg, dir->d_name, match_idx + 1, match, 0);
    CHECK_COND(err != 0, ERR_REGEX, "Reg exec error !");

    char op_name[MAX_NAME_LENGTH];
    memset(op_name, 0, sizeof(char) * MAX_NAME_LENGTH);
    size_t name_len = (size_t)(match[match_idx].rm_eo - match[match_idx].rm_so);
    CHECK_LENGTH(name_len, MAX_NAME_LENGTH);
    strncpy(op_name, dir->d_name + match[match_idx].rm_so, name_len);
    //Check if it exists
    char* path_to_modify;
    float_op_t* op = check_op_already_exists(*count, op_name, floats);
    if(op == NULL) {
        op = &(floats[*count]);
        strncpy(op->name, op_name, MAX_NAME_LENGTH);
        *count = *count + 1;
    }
    if(mode == HDL) {
        path_to_modify = op->hdl_path;
    }
    else {
        path_to_modify = op->script_path;
    }
    strncpy(path_to_modify, float_paths, MAX_PATH_LENGTH);
    size_t len = strlen(path_to_modify);
    CHECK_LENGTH(len, MAX_PATH_LENGTH - 1);
    path_to_modify[len] = '/';
    //TODO: To test !
    path_to_modify[len + 1] = '\0';
    uint16_t max = (uint16_t)(MAX_PATH_LENGTH - strlen(path_to_modify));
    strncpy(path_to_modify + strlen(path_to_modify), dir->d_name, max);
    return ERR_NONE;
}

auto_error_t update_float_op(const char* float_paths, vivado_hls_t* hls) {

    CHECK_PARAM(float_paths);
    CHECK_PARAM(hls);
    
    char hdl_name[MAX_NAME_LENGTH];
    strncpy(hdl_name, hls->fun_name, MAX_NAME_LENGTH);
    uint16_t max = (uint16_t)(MAX_NAME_LENGTH - strlen(hdl_name));
    strncat(hdl_name, ".vhd", max);

    //Reg vhd
    regex_t reg_vhd;
    regmatch_t match[3];
    
    char pattern_vhd[MAX_PATH_LENGTH];
    int written = snprintf(pattern_vhd, MAX_PATH_LENGTH, "%s_([[:alnum:]]*)", hls->fun_name);
    CHECK_LENGTH(written, MAX_PATH_LENGTH);

    int err = regcomp(&reg_vhd, pattern_vhd, REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    //Reg tcl
    regex_t reg_tcl;
    
    char pattern_tcl[MAX_PATH_LENGTH];
    written = snprintf(pattern_tcl, MAX_PATH_LENGTH, "%s_([[:alnum:]]*)_([[:alnum:]]*)", hls->fun_name);
    CHECK_LENGTH(written, MAX_PATH_LENGTH);

    err = regcomp(&reg_tcl, pattern_tcl, REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg compile error !", regfree(&reg_vhd););
    
    DIR *d;
    d = opendir(float_paths);
    CHECK_COND_DO(d == NULL, ERR_FILE, "Could not open dir !", regfree(&reg_vhd); regfree(&reg_tcl););

    uint8_t count = 0;
    float_op_t* floats = calloc(5, sizeof(float_op_t));
    CHECK_COND_DO(floats == NULL, ERR_MEM, "Could not allocate space for float ops !", regfree(&reg_vhd); regfree(&reg_tcl); closedir(d););
    uint8_t last = 5;
    
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if(count == last) {
            float_op_t* new_float_ops = realloc(floats, last * 2);
            CHECK_COND_DO(new_float_ops == NULL, ERR_MEM, "Could not reallocate for float ops !", regfree(&reg_vhd); regfree(&reg_tcl); closedir(d); free(floats););
            floats = new_float_ops;
            last = last * 2;
        }
        if(strncmp(hdl_name, dir->d_name, MAX_NAME_LENGTH) != 0) {
            char* file_ext = strrchr(dir->d_name, '.');
            CHECK_COND(file_ext == NULL, ERR_FILE, "No file ext !");
            if(strncmp(file_ext + 1, "vhd", 3) == 0) {
                CHECK_CALL_DO(check_file(&count, &reg_vhd, match, floats, float_paths, HDL, 1, dir), "check_file failed !", regfree(&reg_vhd); regfree(&reg_tcl); closedir(d); free(floats););
            }
            else if(strncmp(file_ext + 1, "tcl", 3) == 0) {
                CHECK_CALL_DO(check_file(&count, &reg_tcl, match, floats, float_paths, TCL, 2, dir), "check_file failed !", regfree(&reg_vhd); regfree(&reg_tcl); closedir(d); free(floats););
            }
        }
    }

    closedir(d);
    regfree(&reg_vhd);
    regfree(&reg_tcl);
    hls->float_ops = floats;
    hls->nb_float_op = count;
    return ERR_NONE;
    
}

auto_error_t find_float_op(vivado_hls_t* hls) {
    CHECK_PARAM(hls);

    char float_paths[MAX_PATH_LENGTH];
    strncpy(float_paths, hls->project_path, MAX_PATH_LENGTH);
    uint16_t max = (uint16_t)(MAX_PATH_LENGTH - strlen(float_paths) - 1);
    int written = snprintf(float_paths + strlen(float_paths), max, 
        "/solution1/syn/vhdl");
    CHECK_LENGTH(written, max);

    update_float_op(float_paths, hls);

    return ERR_NONE;
}

auto_error_t open_dot_file(vivado_hls_t* hls, hdl_source_t* hdl) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hdl);

    char dot_file_name[MAX_NAME_LENGTH];
    strcpy(dot_file_name, hdl->top_file_name);
    *strrchr(dot_file_name, '.') = '\0';
    strcat(dot_file_name, ".dot");

    char dot_file_path[MAX_PATH_LENGTH];
    strncpy(dot_file_path, hdl->dir, MAX_PATH_LENGTH);
    
    uint16_t max = (uint16_t)(MAX_PATH_LENGTH - strlen(dot_file_path));
    CHECK_COND(max > MAX_PATH_LENGTH, ERR_NAME_TOO_LONG, "Name too long !");
    strncat(dot_file_path, "/../reports/", max);

    max = (uint16_t)(MAX_PATH_LENGTH - strlen(dot_file_path));
    CHECK_COND(max > MAX_PATH_LENGTH, ERR_NAME_TOO_LONG, "Name too long !");
    strncat(dot_file_path, dot_file_name, max);

    char* dot_source = get_source(dot_file_path, NULL);
    CHECK_NULL(dot_source, ERR_FILE, "Couldn't read dot_source");

    regex_t reg;
    regmatch_t match[3];
    char pattern[MAX_NAME_LENGTH]; 
    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t* float_op = &(hls->float_ops[i]);
        char* dot_source_off = dot_source;
        int err = 0;
        
        if(strncmp(float_op->name, "fcmp", 4) == 0) {
            float_op->latency = 0;
        }
        else {
            int written = snprintf(pattern, MAX_NAME_LENGTH, "%s([^(\\v)])*latency=", float_op->name);
            CHECK_COND_DO(written >= MAX_PATH_LENGTH, ERR_NAME_TOO_LONG, "Name too long !", free(dot_source););

            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !", free(dot_source););

            err = regexec(&reg, dot_source, 1, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", free(dot_source); regfree(&reg););

            char* start_match = dot_source + match[0].rm_eo;
            uint8_t dot_latency = (uint8_t)strtol(start_match, NULL, 10);
            float_op->latency = dot_latency - 2;
            regfree(&reg);
        }
        char** name_list = NULL;
        uint8_t last = 0;
        allocate_str_arr(&name_list, &last);
        uint8_t nb_names = 0;
        
        int written = snprintf(pattern, MAX_NAME_LENGTH, "%s([^(\\v)])*op = \"(\\w+)\"", float_op->name);
        CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG, "Name too long !", free(dot_source); free_str_arr(name_list, last);)

        err = regcomp(&reg, pattern, REG_EXTENDED);
        CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !", free(dot_source); free_str_arr(name_list, last););
        while(err == 0) {
            err = regexec(&reg, dot_source_off, 3, match, 0);
            if(err != 0) {
                break;
            }
            else {
                strncpy(name_list[nb_names], dot_source_off + match[2].rm_so, (size_t)(match[2].rm_eo - match[2].rm_so));
                nb_names++;
                dot_source_off += match[0].rm_eo;
            }
        }
        regfree(&reg);
        CHECK_COND_DO(nb_names == 0, ERR_FILE, "At least one name is needed !", free(dot_source); free_str_arr(name_list, last););
        float_op->arith_unit_name_list = name_list;
        float_op->nb_arith_names = nb_names;
    }
    free(dot_source);

    return ERR_NONE;
}

int compare(const void* elem1 , const void* elem2) {
    const float_op_t* op1 = *((const float_op_t**)elem1);
    const float_op_t* op2 = *((const float_op_t**)elem2);
    if(op1->appearance_offset > op2->appearance_offset) {
        return 1;
    }
    if(op1->appearance_offset < op2->appearance_offset) {
        return -1;
    }
    return 0;
}

auto_error_t update_arithmetic_units(project_t* project, vivado_hls_t* hls, axi_ip_t* axi_ip) {
    CHECK_PARAM(project);
    CHECK_PARAM(hls);

    char arithmetic_path[MAX_PATH_LENGTH];
    strcpy(arithmetic_path, axi_ip->path);
    int written = snprintf(arithmetic_path + strlen(arithmetic_path), MAX_NAME_LENGTH, "/%s_1.0/src/arithmetic_units.vhd", axi_ip->name);
    CHECK_LENGTH(written, MAX_NAME_LENGTH);

    char* arithmetic_source = get_source(arithmetic_path, NULL);
    CHECK_NULL(arithmetic_source, ERR_FILE, "Could not read arithmetic source !");
    char* source_off = arithmetic_source;

    float_op_t** sorted_by_appearance = calloc(hls->nb_float_op, sizeof(float_op_t*));
    CHECK_COND_DO(sorted_by_appearance == NULL, ERR_MEM, "Could not allocate memory !", free(arithmetic_source););

    uint8_t count = 0;
    char pattern[MAX_NAME_LENGTH];
    regex_t reg;
    regmatch_t match[2];

    //WARNING: The order should be valid because arith names are put in order in the list
    //TODO: Make it more reliable
    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t* op = &(hls->float_ops[i]);
        snprintf(pattern, MAX_NAME_LENGTH, "%s", op->name);

        int err = regcomp(&reg, pattern, REG_EXTENDED);
        CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !", free(arithmetic_source); free(sorted_by_appearance););

        err = regexec(&reg, source_off, 1, match, 0);
        CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg); free(arithmetic_source); free(sorted_by_appearance););

        op->appearance_offset = (size_t)match[0].rm_so;
        sorted_by_appearance[count++] = op;
        qsort(sorted_by_appearance, count, sizeof(float_op_t*), compare);

        regfree(&reg);
    }

    FILE* arithmetic_file = fopen(arithmetic_path, "w");
    CHECK_COND_DO(arithmetic_path == NULL, ERR_FILE, "Could not open file arithmetic_units.vhd !", free(arithmetic_source); free(sorted_by_appearance););

    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t* op = sorted_by_appearance[i];
        for(uint8_t j = 0; j < op->nb_arith_names; ++j) {
            char* name = op->arith_unit_name_list[j];
            written = snprintf(pattern, MAX_NAME_LENGTH, "entity %s", name);
            CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG, "Name too long !", free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file);)

            int err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !", free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file););

            err = regexec(&reg, source_off, 1, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg); free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file););

            regfree(&reg);

            fwrite(source_off, sizeof(char), (size_t)match[0].rm_eo, arithmetic_file);
            source_off += match[0].rm_eo;

            strncpy(pattern, "component (\\w+) is", MAX_NAME_LENGTH);
            
            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !", free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file););

            err = regexec(&reg, source_off, 2, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg); free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file););

            char component_name[MAX_NAME_LENGTH];
            size_t name_len = (size_t)(match[1].rm_eo - match[1].rm_so);
            strncpy(component_name, source_off + match[1].rm_so, name_len);
            component_name[name_len] = '\0';

            regfree(&reg);

            fwrite(source_off, sizeof(char), (size_t)match[1].rm_so, arithmetic_file);
            source_off += match[1].rm_so;

            char hdl_file_name[MAX_NAME_LENGTH];
            char* last_slash = strrchr(op->hdl_path, '/');
            if(last_slash == NULL) {
                strcpy(hdl_file_name, op->hdl_path);
            }
            else {
                strcpy(hdl_file_name, last_slash + 1);
            }

            *strrchr(hdl_file_name, '.') = '\0';

            fwrite(hdl_file_name, sizeof(char), strlen(hdl_file_name), arithmetic_file);
            source_off += (match[1].rm_eo - match[1].rm_so);
            fwrite(source_off, sizeof(char), (size_t)(match[0].rm_eo - match[1].rm_eo), arithmetic_file);
            source_off = source_off + (size_t)(match[0].rm_eo - match[1].rm_eo);


            written = snprintf(pattern, MAX_NAME_LENGTH, "\\w+[[:space:]]*\\:[[:space:]]*component[[:space:]]*(%s)", component_name);
            CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG, "Name too long !", free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file););

            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

            err = regexec(&reg, source_off, 2, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg); free(arithmetic_source); free(sorted_by_appearance); fclose(arithmetic_file););

            regfree(&reg);
            
            fwrite(source_off, sizeof(char), (size_t)match[1].rm_so, arithmetic_file);
            source_off += match[0].rm_eo;

            fwrite(hdl_file_name, sizeof(char), strlen(hdl_file_name), arithmetic_file);
        }
    }

    fwrite(source_off, sizeof(char), strlen(source_off), arithmetic_file);
    fclose(arithmetic_file);
    free(arithmetic_source);
    free(sorted_by_appearance);


    return ERR_NONE;


}

auto_error_t update_latency(float_op_t* op) {
    CHECK_PARAM(op);
    char* tcl_script = get_source(op->script_path, NULL);
    CHECK_NULL(tcl_script, ERR_FILE, "Could not read tcl_script !");

    //TODO: Do it in a temp file
    FILE* script_file = fopen(op->script_path, "w");
    CHECK_COND_DO(script_file == NULL, ERR_FILE, "Could not open script file !", free(tcl_script););

    char * offset = tcl_script;

    regex_t reg;
    regmatch_t match[3];

    int err = regcomp(&reg, "c_latency([[:space:]]*)([[:digit:]]*)", REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Regex comp failed !", free(tcl_script); fclose(script_file););

    err = regexec(&reg, tcl_script, 3, match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg); free(tcl_script); fclose(script_file););

    offset += match[2].rm_so;
    fwrite(tcl_script, sizeof(char), (size_t)(offset - tcl_script), script_file);

    char latency_str[MAX_NAME_LENGTH];
    snprintf(latency_str, MAX_NAME_LENGTH, "%" PRIu8, op->latency);

    fwrite(latency_str, sizeof(char), strlen(latency_str), script_file);
    offset = tcl_script + match[0].rm_eo;

    fwrite(offset, sizeof(char), strlen(offset), script_file);

    fclose(script_file);

    free(tcl_script);

    return ERR_NONE;
}

auto_error_t update_fop_tcl(vivado_hls_t* hls) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hls->float_ops);

    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t* op = &(hls->float_ops[i]);
        update_latency(op);
    }

    return ERR_NONE;
}

void free_floats(vivado_hls_t* hls) {
    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        if(hls->float_ops[i].arith_unit_name_list != NULL) {
            free(hls->float_ops[i].arith_unit_name_list);
        }
    }
    free(hls->float_ops);
}

auto_error_t hls_free(vivado_hls_t* hls) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hls->hls_source);

    free(hls->hls_source);
    free_floats(hls);
    return ERR_NONE;
}
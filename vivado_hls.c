#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>
#include <unistd.h>
#include <ftw.h>
#include <dirent.h>
#include "vivado_hls.h"

error_t create_hls(vivado_hls_t* hls, hdl_source_t* hdl_source) {
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
    snprintf(pattern, MAX_NAME_LENGTH, "(.*)_optimized");

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
    
    int written = snprintf(path_temp + strlen(path_temp), max, "/../src/%s.cpp", cpp_name);
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

error_t advance_in_file_hls(regex_t* reg, regmatch_t* match, const char** source_off, const char* pattern, const char** str_match,
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

error_t parse_hls(vivado_hls_t* hls, hdl_source_t* hdl_source) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hls->hls_source);
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->arrays);

    regex_t reg;
    regmatch_t match[2];

    const char* source_off = hls->hls_source;
    const char* str_match;
    size_t str_match_len;

    advance_in_file_hls(&reg, match, &source_off, "return", &str_match, &str_match_len);

    advance_in_file_hls(&reg, match, &source_off, "}", &str_match, &str_match_len);

    size_t end = (size_t)(source_off - hls->hls_source) + 1;
    hls->hls_source[end] = '\0';
    char* new_source = realloc(hls->hls_source, strlen(hls->hls_source) + 1);
    CHECK_NULL(new_source, ERR_MEM, "Realloc failed on hls source !");

    hls->hls_source = new_source;

    source_off = hls->hls_source;

    int err = regcomp(&reg, "\\w+\\s+(\\w+)\\(", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    err = regexec(&reg, source_off, 2, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););

    //source_off = (const char*)(source_off + match[0].rm_so);

    regfree(&reg);


    size_t sub_match_len = (size_t)(match[1].rm_eo - match[1].rm_so);
    CHECK_COND_DO(sub_match_len >= MAX_NAME_LENGTH, ERR_REGEX, "Name too long !", regfree(&reg););
    strncpy(hls->fun_name, source_off + match[1].rm_so, sub_match_len);
    hls->fun_name[sub_match_len] = '\0';

    str_match = source_off + match[0].rm_so;
    str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    source_off = (const char*)(source_off + match[0].rm_so + str_match_len);

    

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
        if(err == REG_NOMATCH) {
            hdl_source->arrays[i].read = true;
            hdl_source->arrays[i].write = true;
        }
        else {
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
                hdl_source->arrays[i].read = true;
                hdl_source->arrays[i].write = true;
            }
        }

    }

    return ERR_NONE;
}

error_t launch_hls_script() {

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

error_t check_file(uint8_t* count, regex_t* reg, regmatch_t* match, float_op_t* floats, const char* float_paths, mode_t mode, uint8_t match_idx, 
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
    path_to_modify[strlen(path_to_modify)] = '/';
    uint16_t max = (uint16_t)(MAX_PATH_LENGTH - strlen(path_to_modify));
    strncpy(path_to_modify + strlen(path_to_modify), dir->d_name, max);
    return ERR_NONE;
}

error_t update_float_op(const char* float_paths, vivado_hls_t* hls) {

    CHECK_PARAM(float_paths);
    CHECK_PARAM(hls);

    DIR *d;
    d = opendir(float_paths);
    CHECK_COND(d == NULL, ERR_FILE, "Could not open dir !");
    uint8_t count = 0;
    float_op_t* floats = calloc(5, sizeof(float_op_t));
    CHECK_COND(floats == NULL, ERR_MEM, "Could not allocate space for float ops !");
    
    
    char hdl_name[MAX_NAME_LENGTH];
    strncpy(hdl_name, hls->fun_name, MAX_NAME_LENGTH);
    uint16_t max = (uint16_t)(MAX_NAME_LENGTH - strlen(hdl_name));
    strncat(hdl_name, ".vhd", max);
    uint8_t last = 5;

    //Reg vhd
    regex_t reg_vhd;
    regmatch_t match[3];
    
    char pattern_vhd[MAX_PATH_LENGTH];
    snprintf(pattern_vhd, MAX_PATH_LENGTH, "%s_([[:alnum:]]*)", hls->fun_name);

    int err = regcomp(&reg_vhd, pattern_vhd, REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    //Reg tcl
    regex_t reg_tcl;
    
    char pattern_tcl[MAX_PATH_LENGTH];
    snprintf(pattern_tcl, MAX_PATH_LENGTH, "%s_([[:alnum:]]*)_([[:alnum:]]*)", hls->fun_name);

    err = regcomp(&reg_tcl, pattern_tcl, REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    if (d != NULL) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if(count == last) {
                float_op_t* new_float_ops = realloc(floats, last * 2);
                CHECK_COND_DO(new_float_ops == NULL, ERR_MEM, "Could not reallocate for float ops !", regfree(&reg_vhd); regfree(&reg_tcl););
                floats = new_float_ops;
                last = last * 2;
            }
            if(strncmp(hdl_name, dir->d_name, MAX_NAME_LENGTH) != 0) {
                char* file_ext = strrchr(dir->d_name, '.');
                CHECK_COND(file_ext == NULL, ERR_FILE, "No file ext !");
                if(strncmp(file_ext + 1, "vhd", 3) == 0) {
                    check_file(&count, &reg_vhd, match, floats, float_paths, HDL, 1, dir);
                }
                else if(strncmp(file_ext + 1, "tcl", 3) == 0) {
                    check_file(&count, &reg_tcl, match, floats, float_paths, TCL, 2, dir);
                }
            }
        }
        closedir(d);
    }
    regfree(&reg_vhd);
    regfree(&reg_tcl);
    hls->float_ops = floats;
    hls->nb_float_op = count;
    return ERR_NONE;
    
}

error_t find_float_op(vivado_hls_t* hls) {
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

void free_str(char** files_to_add, uint8_t last) {
    if(files_to_add == NULL) {
        return;
    }
    for(uint8_t i = 0; i < last; ++i) {
        if(files_to_add[i] != NULL) {
            free(files_to_add[i]);
        }
    }
    free(files_to_add);
}

error_t allocate_str(char*** files_to_add, uint8_t* last) {
    CHECK_PARAM(files_to_add);
    //CHECK_PARAM(*files_to_add);
    CHECK_PARAM(last);
    char** new_files_to_add;
    uint8_t prev_last = *last;
    if(*last == 0) {
        new_files_to_add = calloc(5, sizeof(char*));
        *last = 5;
    }
    else {
        new_files_to_add = realloc(*files_to_add, *last * 2 * sizeof(char*));
        *last = *last * 2;
    }
    CHECK_COND_DO(new_files_to_add == NULL, ERR_MEM, "Could not realloc for files_to_add !", free_str(*files_to_add, prev_last););
    *files_to_add = new_files_to_add;

    for(uint8_t i = prev_last; i < *last; ++i) {
        (*files_to_add)[i] = calloc(MAX_PATH_LENGTH, sizeof(char));
        CHECK_COND_DO((*files_to_add)[i] == NULL, ERR_MEM, "Could not realloc for files_to_add !", free_str(*files_to_add, i););
    }
    return ERR_NONE;
}

error_t open_dot_file(vivado_hls_t* hls, hdl_source_t* hdl) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hdl);

    char dot_file_name[MAX_NAME_LENGTH];
    strcpy(dot_file_name, hdl->top_file_name);
    *strrchr(dot_file_name, '.') = '\0';
    strcat(dot_file_name, ".dot");

    //TODO: Check length
    char dot_file_path[MAX_PATH_LENGTH];
    strcpy(dot_file_path, hdl->dir);
    //TODO: On every max var do the following;
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
            snprintf(pattern, MAX_NAME_LENGTH, "%s([^(\\v)])*latency=", float_op->name);
            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");
            err = regexec(&reg, dot_source, 1, match, 0);
            CHECK_COND(err != 0, ERR_REGEX, "Regex exec failed !");
            char* start_match = dot_source + match[0].rm_eo;
            uint8_t dot_latency = (uint8_t)strtol(start_match, NULL, 10);
            float_op->latency = dot_latency - 2;
            regfree(&reg);
        }
        char** name_list = NULL;
        uint8_t last = 0;
        allocate_str(&name_list, &last);
        uint8_t nb_names = 0;
        snprintf(pattern, MAX_NAME_LENGTH, "%s([^(\\v)])*op = \"(\\w+)\"", float_op->name);
        err = regcomp(&reg, pattern, REG_EXTENDED);
        CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");
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
        CHECK_COND(nb_names == 0, ERR_FILE, "At least one name is needed !");
        float_op->arith_unit_name_list = name_list;
        float_op->nb_arith_names = nb_names;
    }
    return ERR_NONE;
}


error_t update_latency(float_op_t* op) {
    CHECK_PARAM(op);
    char* tcl_script = get_source(op->script_path, NULL);
    FILE* script_file = fopen(op->script_path, "w");
    char * offset = tcl_script;

    regex_t reg;
    regmatch_t match[3];

    int err = regcomp(&reg, "c_latency([[:space:]]*)([[:digit:]]*)", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Regex comp failed !");

    err = regexec(&reg, tcl_script, 3, match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg););

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

error_t update_fop_tcl(vivado_hls_t* hls) {
    CHECK_PARAM(hls);

    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t* op = &(hls->float_ops[i]);
        update_latency(op);
    }

    return ERR_NONE;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

error_t hls_free(vivado_hls_t* hls) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hls->hls_source);

    free(hls->hls_source);
    nftw("vivado_hls", unlink_cb, 64, FTW_DEPTH | FTW_PHYS);

    return ERR_NONE;
}
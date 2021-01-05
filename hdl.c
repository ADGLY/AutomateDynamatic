#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "hdl.h"

static const char* const write_ports[NB_BRAM_INTERFACE] = {
    "_address0", "_ce0", "_we0", "_dout0", "_din0"
};

static const char* const read_ports[NB_BRAM_INTERFACE] = {
    "_address1", "_ce1", "_we1", "_dout1", "_din1"
};


auto_error_t get_simple_info(hdl_source_t* hdl_source, const char* pattern, regmatch_t* match, size_t nmatch) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->source);

    regex_t reg;
    int err = regcomp(&reg, pattern, REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");
    err = regexec(&reg, hdl_source->source, nmatch, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););
    regfree(&reg);

    return ERR_NONE;
}

auto_error_t get_end_of_ports_decl(hdl_source_t* hdl_source) {
    regmatch_t match[1];
    CHECK_CALL(get_simple_info(hdl_source, "end;", match, 1), "get_simple_info failed !");
    hdl_source->end_of_ports_decl = (size_t)match[0].rm_eo;

    return ERR_NONE;
}

auto_error_t get_entity_name(hdl_source_t* hdl_source) {
    regmatch_t match[2];
    CHECK_CALL(get_simple_info(hdl_source, "entity[[:space:]]*(\\w+)[[:space:]]*is", match, 2), "get_simple_info failed !");
    CHECK_LENGTH((size_t)(match[1].rm_eo - match[1].rm_so), MAX_NAME_LENGTH);
    strncpy(hdl_source->name, hdl_source->source +match[1].rm_so, (size_t)(match[1].rm_eo - match[1].rm_so));

    return ERR_NONE;
}

auto_error_t iterate_hdl(regex_t* reg, const char** source_off, regmatch_t* match, const char** str_match, size_t* str_match_len, int* err, size_t* array_count) {
    *err = regexec(reg, *source_off, 2, (regmatch_t*)match, 0);
    if(array_count == NULL) {
        CHECK_COND_DO(*err != 0, ERR_REGEX, "Reg exec error !", regfree(reg););
    }
    else {
        CHECK_COND_DO(*err != 0 && *array_count > 1, ERR_REGEX, "Reg exec error !", regfree(reg););
    }
    
    *str_match = *source_off + match[0].rm_so;
    *str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    *source_off = (const char*)(*source_off + match[0].rm_so + *str_match_len);

    return ERR_NONE;
}

auto_error_t get_arrays(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->source);

    regex_t reg;
    regmatch_t match[2];
    const char* source_off = hdl_source->source;

    size_t alloc_size = 10;
    hdl_array_t* arrays = calloc(alloc_size, sizeof(hdl_array_t));
    CHECK_NULL(arrays, ERR_MEM, "Failed to allocate memory for arrays !");
    size_t array_count = 0;

    size_t end_of_port = hdl_source->end_of_ports_decl;

    int err = regcomp(&reg, "(\\w+)_din1", REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg compile error !", free((void*)arrays););

    const char* str_match;
    size_t str_match_len;
    CHECK_CALL_DO(iterate_hdl(&reg, &source_off, match, &str_match, &str_match_len, &err, NULL), "iterate_hdl_failed !", free((void*)arrays););

    size_t end_arrays = 0;

    while((size_t)(source_off - hdl_source->source) <= end_of_port || err != 0) {

        CHECK_COND_DO((size_t)(match[1].rm_eo - match[1].rm_so) >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG, "", regfree(&reg); free((void*)arrays););
        strncpy(arrays[array_count].name, str_match, (size_t)(match[1].rm_eo - match[1].rm_so));
        array_count++;
        
        if(array_count == alloc_size) {
            alloc_size*=2;
            hdl_array_t* new_arrays = realloc(arrays, alloc_size * sizeof(hdl_array_t));
            CHECK_COND_DO(new_arrays == NULL, ERR_MEM, "Failed to realloc !", regfree(&reg); free(arrays););
            arrays = new_arrays;
        }
        
        end_arrays = (size_t)(source_off - hdl_source->source);

        CHECK_CALL_DO(iterate_hdl(&reg, &source_off, match, &str_match, &str_match_len, &err, &array_count), "iterate_hdl_failed !", free((void*)arrays););
    }
    if(array_count == 0) {
        hdl_source->arrays = NULL;
        hdl_source->nb_arrays = array_count;
    }
    else {
        hdl_array_t* new_arrays = realloc(arrays, array_count * sizeof(hdl_array_t));
        CHECK_COND_DO(new_arrays == NULL, ERR_MEM, "Failed to realloc !", regfree(&reg); free(arrays));
        hdl_source->arrays = new_arrays;
        hdl_source->nb_arrays = array_count;
        hdl_source->end_arrays = end_arrays;
    }
    regfree(&reg);
    return ERR_NONE;
}

auto_error_t fill_interfaces(bram_interface_t* read_interface, bram_interface_t* write_interface, const char* arr_name) {
    CHECK_PARAM(read_interface);
    CHECK_PARAM(write_interface);
    CHECK_PARAM(arr_name);

    char* pointer_to_read = (char*)read_interface;
    char* pointer_to_write = (char*)write_interface;

    for(int i = 0; i < NB_BRAM_INTERFACE; ++i) {
        size_t name_len = strlen(arr_name);
        
        size_t read_total_len = name_len + strlen(read_ports[i]) + 1;
        CHECK_LENGTH(read_total_len, MAX_NAME_LENGTH);

        strncpy(pointer_to_read, arr_name, name_len);
        strncpy(pointer_to_read + name_len, read_ports[i], strlen(read_ports[i]) + 1);

        size_t write_total_len = name_len + strlen(write_ports[i]) + 1;
        CHECK_LENGTH(write_total_len, MAX_NAME_LENGTH);

        strncpy(pointer_to_write, arr_name, name_len);
        strncpy(pointer_to_write + name_len, write_ports[i], strlen(write_ports[i]) + 1);

        pointer_to_read += MAX_NAME_LENGTH;
        pointer_to_write += MAX_NAME_LENGTH;
    }
    return ERR_NONE;
}

auto_error_t fill_arrays_ports(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->arrays);

    for(size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        hdl_array_t* array = &(hdl_source->arrays[i]);
        const char* arr_name = array->name;
        CHECK_CALL(fill_interfaces(&(array->read_ports), &(array->write_ports), arr_name), "fill_interfaces failed !");
    }
    return ERR_NONE;
}

auto_error_t get_params(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->source);


    regex_t reg;
    regmatch_t match[2];
    const char* source_off = hdl_source->source + hdl_source->end_arrays;
    size_t alloc_size = 10;
    hdl_in_param_t* params = calloc(alloc_size, sizeof(hdl_in_param_t));
    CHECK_NULL(params, ERR_MEM, "Failed to alloc for params !");
    size_t param_count = 0;
    size_t end_of_port = hdl_source->end_of_ports_decl;


    int err = regcomp(&reg, "(\\w+)_din", REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg compile error !", free(params););

    const char* str_match;
    size_t str_match_len;
    CHECK_CALL_DO(iterate_hdl(&reg, &source_off, match, &str_match, &str_match_len, &err, NULL), "iterate_hdl_failed !", free((void*)params););


    while((size_t)(source_off - hdl_source->source) <= end_of_port || err != 0) {
        size_t name_len = (size_t)(match[1].rm_eo - match[1].rm_so);
        CHECK_CALL_DO(name_len >= MAX_NAME_LENGTH, "Name too long !", regfree(&reg); free(params););
        strncpy(params[param_count].name, str_match, name_len);
        param_count++;

        if(param_count == alloc_size) {
            hdl_in_param_t* new_params = realloc(params, sizeof(hdl_in_param_t) * alloc_size * 2);
            CHECK_COND_DO(new_params == NULL, ERR_MEM, "Realloc failed for params !", regfree(&reg); free(params););
            params = new_params;
            alloc_size *= 2;
        }

        CHECK_CALL_DO(iterate_hdl(&reg, &source_off, match, &str_match, &str_match_len, &err, NULL), "iterate_hdl_failed !", free((void*)params););
    }
    if(param_count == 0) {
        hdl_source->params = NULL;
        hdl_source->nb_params = param_count;
    }
    else {
        hdl_in_param_t* new_params = realloc(params, sizeof(hdl_in_param_t) * param_count);
        CHECK_COND_DO(new_params == NULL, ERR_MEM, "Realloc failed for params !", regfree(&reg); free(params));
        hdl_source->params = new_params;
        hdl_source->nb_params = param_count;
    }
    regfree(&reg);
    return ERR_NONE;
}

auto_error_t hdl_create(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);

    memset(hdl_source, 0, sizeof(hdl_source_t));

    char* result = getcwd(hdl_source->exec_path, MAX_PATH_LENGTH);
    CHECK_COND(result == NULL, ERR_IO, "getcwd error !");

    CHECK_CALL(get_path(hdl_source->dir, "What is the directory of the Dynamatic output (hdl) ?"), "get_path failed !");

    CHECK_CALL(get_name(hdl_source->top_file_name, "What is the name of the top file ?"), "get_name failed !");

    strcpy(hdl_source->top_file_path, hdl_source->dir);
    hdl_source->top_file_path[strlen(hdl_source->dir)] = '/';
    if(strlen(hdl_source->top_file_name) + strlen(hdl_source->dir) + 2 >= MAX_PATH_LENGTH) {
        fprintf(stderr, "Name too long !\n");
    }
    strcpy(hdl_source->top_file_path + strlen(hdl_source->dir) + 1, hdl_source->top_file_name);
    return ERR_NONE;
}

auto_error_t get_end_out_width(hdl_source_t* hdl_source) {
    regmatch_t match[2];
    CHECK_CALL(get_simple_info(hdl_source, "end_out[[:space:]]*:[[:space:]]*out[[:space:]]*std_logic_vector[[:space:]]*\\(([[:digit:]]+)", match, 2), "get_simple_info failed !");
    char width[MAX_NAME_LENGTH];
    size_t len = (size_t)(match[1].rm_eo - match[1].rm_so);
    CHECK_LENGTH(len - 1, MAX_NAME_LENGTH);

    strncpy(width, hdl_source->source + match[1].rm_so, len);
    width[len] = '\0';
    hdl_source->end_out_width = (size_t)(strtoll(width, NULL, 10) + 1);
    return ERR_NONE;
}

auto_error_t parse_hdl(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    char* source = get_source(hdl_source->top_file_path, NULL);
    CHECK_NULL(source, ERR_IO, "Did not manage to read source file !");
    hdl_source->source = source;

    CHECK_CALL(get_end_of_ports_decl(hdl_source), "get_end_of_ports_decl failed !");
    CHECK_CALL(get_end_out_width(hdl_source), "get_end_out_width failed !");
    CHECK_CALL(get_entity_name(hdl_source), "get_entity_name failed !");
    CHECK_CALL(get_arrays(hdl_source), "get_arrays failed !");
    CHECK_CALL(fill_arrays_ports(hdl_source), "fill_arrays_ports failed !");
    CHECK_CALL(get_params(hdl_source), "get_params failed !");
    return ERR_NONE;
}

auto_error_t hdl_free(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    if(hdl_source->arrays != NULL) {
        free(hdl_source->arrays);
        hdl_source->arrays = NULL;
    }
    if(hdl_source->params != NULL) {
        free(hdl_source->params);
        hdl_source->params = NULL;
    }
    if(hdl_source->source != NULL) {
        free(hdl_source->source);
        hdl_source->source = NULL;
    }
    return ERR_NONE;
}
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "hdl.h"
#include "vivado.h"

static const char* const write_ports[NB_BRAM_INTERFACE] = {
    "_address0", "_ce0", "_we0", "_dout0", "_din0"
};

static const char* const read_ports[NB_BRAM_INTERFACE] = {
    "_address1", "_ce1", "_we1", "_dout1", "_din1"
};

error_t get_end_of_ports_decl(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->source);

    regex_t reg;
    regmatch_t match[1];
    int err = regcomp(&reg, "end;", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");
    err = regexec(&reg, hdl_source->source, 1, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););
    hdl_source->end_of_ports_decl = (size_t)match[0].rm_eo;
    regfree(&reg);
    return ERR_NONE;
}

error_t get_entity_name(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->source);
    CHECK_PARAM(hdl_source->name);

    regex_t reg;
    regmatch_t match[2];
    int err = regcomp(&reg, "entity[ ](\\w+)[ ]is", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");
    err = regexec(&reg, hdl_source->source, 2, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg));
    regfree(&reg);
    CHECK_LENGTH((size_t)(match[1].rm_eo - match[1].rm_so), MAX_NAME_LENGTH);
    strncpy(hdl_source->name, hdl_source->source +match[1].rm_so, (size_t)(match[1].rm_eo - match[1].rm_so));

    return ERR_NONE;
}

error_t get_arrays(hdl_source_t* hdl_source) {
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

    int err = regcomp(&reg, "(\\w+)_address0", REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg compile error !", free((void*)arrays););

    //First part of the while loop
    err = regexec(&reg, source_off, 2, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg); free((void*)arrays););
    
    const char* str_match = source_off + match[0].rm_so;
    size_t str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    source_off = (const char*)(source_off + match[0].rm_so + str_match_len);

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

        err = regexec(&reg, source_off, 2, (regmatch_t*)match, 0);
        CHECK_COND(err != 0 && array_count > 1, ERR_REGEX, "Reg exec error !");

        str_match = source_off + match[0].rm_so;
        str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
        source_off = (const char*)(source_off + match[0].rm_so + str_match_len);
    }
    hdl_array_t* new_arrays = realloc(arrays, array_count * sizeof(hdl_array_t));
    CHECK_COND_DO(new_arrays == NULL, ERR_MEM, "Failed to realloc !", regfree(&reg); free(arrays));
    hdl_source->arrays = new_arrays;
    hdl_source->nb_arrays = array_count;
    regfree(&reg);
    return ERR_NONE;
}

error_t fill_interfaces(bram_interface_t* read_interface, bram_interface_t* write_interface, const char* arr_name) {
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

error_t fill_arrays_ports(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->arrays);
    CHECK_PARAM(hdl_source->name);

    for(size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        hdl_array_t* array = &(hdl_source->arrays[i]);
        const char* arr_name = array->name;
        CHECK_CALL(fill_interfaces(&(array->read_ports), &(array->write_ports), arr_name), "fill_interfaces failed !");
    }
    return ERR_NONE;
}

error_t get_params(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    CHECK_PARAM(hdl_source->source);


    regex_t reg;
    regmatch_t match[2];
    const char* source_off = hdl_source->source;
    size_t alloc_size = 10;
    hdl_in_param_t* params = calloc(alloc_size, sizeof(hdl_in_param_t));
    CHECK_NULL(params, ERR_MEM, "Failed to alloc for params !");
    size_t param_count = 0;
    size_t end_of_port = hdl_source->end_of_ports_decl;


    int err = regcomp(&reg, "(\\w+)_din[[:space:]]", REG_EXTENDED);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg compile error !", free(params););


    //first iter
    err = regexec(&reg, source_off, 2, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg); free(params););

    const char* str_match = source_off + match[0].rm_so;
    size_t str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    source_off = (const char*)(source_off + match[0].rm_so + str_match_len);


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

        //TODO: Hacky
        err = regexec(&reg, source_off, 2, (regmatch_t*)match, 0);
        if(err != 0) {
            break;
        }

        str_match = source_off + match[0].rm_so;
        str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
        source_off = (const char*)(source_off + match[0].rm_so + str_match_len);
    }
    hdl_in_param_t* new_params = realloc(params, sizeof(hdl_in_param_t) * param_count);
    CHECK_COND_DO(new_params == NULL, ERR_MEM, "Realloc failed for params !", regfree(&reg); free(params));
    hdl_source->params = new_params;
    hdl_source->nb_params = param_count;
    regfree(&reg);
    return ERR_NONE;
}

error_t hdl_create(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);

    memset(hdl_source, 0, sizeof(hdl_source_t));

    char* result = getcwd(hdl_source->exec_path, MAX_PATH_LENGTH);
    CHECK_COND(result == NULL, ERR_FILE, "getcwd error !");

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

error_t parse_hdl(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);
    char* source = get_source(hdl_source->top_file_path, NULL);
    CHECK_NULL(source, ERR_FILE, "Did not manage to read source file !");
    hdl_source->source = source;

    CHECK_CALL(get_end_of_ports_decl(hdl_source), "get_end_of_ports_decl failed !");
    CHECK_CALL(get_entity_name(hdl_source), "get_entity_name failed !");
    CHECK_CALL(get_arrays(hdl_source), "get_arrays failed !");
    CHECK_CALL(fill_arrays_ports(hdl_source), "fill_arrays_ports failed !");
    CHECK_CALL(get_params(hdl_source), "get_params failed !");
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

error_t update_arithmetic_units(project_t* project, vivado_hls_t* hls) {
    CHECK_PARAM(project);
    CHECK_PARAM(hls);

    char arithmetic_path[MAX_PATH_LENGTH];
    strcpy(arithmetic_path, project->axi_ip.path);
    snprintf(arithmetic_path + strlen(arithmetic_path), MAX_NAME_LENGTH, "/%s_1.0/src/arithmetic_units.vhd", project->axi_ip.name);
    char* arithmetic_source = get_source(arithmetic_path, NULL);
    CHECK_NULL(arithmetic_source, ERR_FILE, "Could not read arithmetic source !");
    char* source_off = arithmetic_source;

    float_op_t** sorted_by_appearance = calloc(hls->nb_float_op, sizeof(float_op_t*));
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
        CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

        err = regexec(&reg, source_off, 1, match, 0);
        CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg););

        op->appearance_offset = (size_t)match[0].rm_so;
        sorted_by_appearance[count++] = op;
        qsort(sorted_by_appearance, count, sizeof(float_op_t*), compare);

        regfree(&reg);
    }

    FILE* arithmetic_file = fopen(arithmetic_path, "w");
    CHECK_COND_DO(arithmetic_path == NULL, ERR_FILE, "Could not open file arithmetic_units.vhd !", free(source_off););

    for(uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t* op = sorted_by_appearance[i];
        for(uint8_t j = 0; j < op->nb_arith_names; ++j) {
            char* name = op->arith_unit_name_list[j];
            snprintf(pattern, MAX_NAME_LENGTH, "entity %s", name);

            int err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

            err = regexec(&reg, source_off, 1, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg););

            regfree(&reg);

            fwrite(source_off, sizeof(char), (size_t)match[0].rm_eo, arithmetic_file);
            source_off += match[0].rm_eo;

            snprintf(pattern, MAX_NAME_LENGTH, "component (\\w+) is");
            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

            err = regexec(&reg, source_off, 2, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg););

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


            snprintf(pattern, MAX_NAME_LENGTH, "\\w+[[:space:]]*\\:[[:space:]]*component[[:space:]]*(%s)", component_name);
            err = regcomp(&reg, pattern, REG_EXTENDED);
            CHECK_COND(err != 0, ERR_REGEX, "Regex compilation failed !");

            err = regexec(&reg, source_off, 2, match, 0);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !", regfree(&reg););

            regfree(&reg);
            
            fwrite(source_off, sizeof(char), (size_t)match[1].rm_so, arithmetic_file);
            source_off += match[0].rm_eo;

            fwrite(hdl_file_name, sizeof(char), strlen(hdl_file_name), arithmetic_file);
        }
    }

    fwrite(source_off, sizeof(char), strlen(source_off), arithmetic_file);
    fclose(arithmetic_file);
    free(arithmetic_source);


    return ERR_NONE;


}

error_t hdl_free(hdl_source_t* hdl_source) {
    CHECK_PARAM(hdl_source);

    free(hdl_source->arrays);
    free(hdl_source->params);
    free(hdl_source->source);
    return ERR_NONE;
}
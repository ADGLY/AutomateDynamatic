#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hdl.h"

static const char* const write_ports[NB_BRAM_INTERFACE] = {
    "_address0", "_ce0", "_we0", "_dout0", "_din0"
};

static const char* const read_ports[NB_BRAM_INTERFACE] = {
    "_address1", "_ce1", "_we1", "_dout1", "_din1"
};

void get_end_of_ports_decl(hdl_source_t* hdl_source) {
    regex_t reg;
    regmatch_t match[1];
    int err = regcomp(&reg, "end;", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
        return;
    }
    err = regexec(&reg, hdl_source->source, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
         return;
    }
    hdl_source->end_of_ports_decl = match[0].rm_eo;
    regfree(&reg);
}

void get_entity_name(hdl_source_t* hdl_source) {
    regex_t reg;
    regmatch_t match[1];
    int err = regcomp(&reg, "entity[ ]\\w+[ ]is", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
        return;
    }
    err = regexec(&reg, hdl_source->source, 1, (regmatch_t*)match, 0);
    if(err != 0) {
        fprintf(stderr, "Reg exec error !\n");
        return;
    }

    #define NAME_START_OFF 7
    #define NAME_END 10
    char* str_match = (char*)(hdl_source->source + match[0].rm_so);
    const size_t str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    char* name_start = (char*)(str_match + NAME_START_OFF);
    const size_t name_len = str_match_len - NAME_END;
    if(name_len >= MAX_NAME_LENGTH) {
        fprintf(stderr, "Name of the entity is too long !\n");
    }
    strncpy(hdl_source->name, name_start, name_len);
    hdl_source->name[name_len + 1] = '\0';
    regfree(&reg);
}

void get_arrays(hdl_source_t* hdl_source) {
    regex_t reg;
    regmatch_t match[1];
    const char* source_off = hdl_source->source;

    size_t alloc_size = 100;
    hdl_array_t* arrays = calloc(alloc_size, sizeof(hdl_array_t));
    if(arrays == NULL) {
        fprintf(stderr, "Failed to allocate memory for arrays !\n");
        return;
    }
    size_t array_count = 0;

    size_t end_of_port = hdl_source->end_of_ports_decl;

    int err = regcomp(&reg, "\\w+_address0", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
        return;
    }

    //First part of the while loop
    err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
        fprintf(stderr, "Reg exec error !\n");
    }
    const char* str_match = source_off + match[0].rm_so;
    size_t str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    source_off += match[0].rm_so + str_match_len;

    while((size_t)(source_off - hdl_source->source) <= end_of_port || err != 0) {

        size_t name_len = str_match_len - 9;
        if(name_len >= MAX_NAME_LENGTH) {
            fprintf(stderr, "Name of the array is too long : %s !\n", str_match);
            return;
        }
        strncpy(arrays[array_count].name, str_match, name_len);
        arrays[array_count].name[name_len + 1] = '\0';
        array_count++;

        if(array_count == alloc_size) {
            alloc_size*=2;
            hdl_array_t* new_arrays = realloc(arrays, alloc_size * sizeof(hdl_array_t));
            if(new_arrays == NULL) {
                fprintf(stderr, "Realloc failed for arrays !\n");
                break;
            }
            else {
                arrays = new_arrays;
            }
        }

        err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
        if(err != 0) {
            if(array_count > 1 ) {
                fprintf(stderr, "Reg exec error !\n");
            }
            return;
        }

        str_match = source_off + match[0].rm_so;
        str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
        source_off += match[0].rm_so + str_match_len;
    }
    hdl_array_t* new_arrays = realloc(arrays, array_count * sizeof(hdl_array_t));
    if(new_arrays == NULL) {
        fprintf(stderr, "Failed to realloc for arrays !\n");
        hdl_source->arrays = arrays;
    }
    else {
        hdl_source->arrays = new_arrays;
    }
    hdl_source->nb_arrays = array_count;
    regfree(&reg);
}

void fill_interfaces(bram_interface_t* read_interface, bram_interface_t* write_interface, const char* arr_name) {

    char* pointer_to_read = (char*)read_interface;
    char* pointer_to_write = (char*)write_interface;

    for(int i = 0; i < NB_BRAM_INTERFACE; ++i) {
        size_t name_len = strlen(arr_name);
        
        size_t read_total_len = name_len + strlen(read_ports[i]) + 1;
        if(read_total_len >= MAX_NAME_LENGTH) {
            fprintf(stderr, "Name is too long !\n");
            return;
        }

        strncpy(pointer_to_read, arr_name, name_len);
        strncpy(pointer_to_read + name_len, read_ports[i], strlen(read_ports[i]) + 1);

        size_t write_total_len = name_len + strlen(write_ports[i]) + 1;
        if(write_total_len >= MAX_NAME_LENGTH) {
            fprintf(stderr, "Name is too long !\n");
            return;
        }

        strncpy(pointer_to_write, arr_name, name_len);
        strncpy(pointer_to_write + name_len, write_ports[i], strlen(write_ports[i]) + 1);

        pointer_to_read += MAX_NAME_LENGTH;
        pointer_to_write += MAX_NAME_LENGTH;
    }
}

void fill_arrays_ports(hdl_source_t* hdl_source) {
    for(size_t i = 0; i < hdl_source->nb_arrays; ++i) {
        hdl_array_t* array = &(hdl_source->arrays[i]);
        const char* arr_name = array->name;
        fill_interfaces(&(array->read_ports), &(array->write_ports), arr_name);
    }
}

void get_params(hdl_source_t* hdl_source) {
    regex_t reg;
    regmatch_t match[1];
    const char* source_off = hdl_source->source;
    size_t alloc_size = 100;
    hdl_in_param_t* params = calloc(alloc_size, sizeof(hdl_in_param_t));
    size_t param_count = 0;
    size_t end_of_port = hdl_source->end_of_ports_decl;


    int err = regcomp(&reg, "\\w+_din ", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }


    //first iter
    err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
        return;
    }
    const char* str_match = source_off + match[0].rm_so;
    size_t str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
    source_off += match[0].rm_so + str_match_len;


    while((size_t)(source_off - hdl_source->source) <= end_of_port) {
        size_t name_len = str_match_len - 5;
        strncpy(params[param_count].name, str_match, name_len);
        params[param_count].name[name_len + 1] = '\0';
        param_count++;

        if(param_count == alloc_size) {
            hdl_in_param_t* new_params = realloc(params, sizeof(hdl_in_param_t) * alloc_size * 2);
            if(new_params == NULL) {
                fprintf(stderr, "Reallocation failed for params !\n");
                return;
            }
            params = new_params;
            alloc_size *= 2;
        }

        err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
        if(err != 0) {
            if(param_count > 1) {
                fprintf(stderr, "Reg exec error !\n");
            }
            break;
        }

        str_match = source_off + match[0].rm_so;
        str_match_len = (size_t)(match[0].rm_eo - match[0].rm_so);
        source_off += match[0].rm_so + str_match_len;
    }
    hdl_in_param_t* new_params = realloc(params, sizeof(hdl_in_param_t) * param_count);
    if(new_params == NULL) {
        fprintf(stderr, "Realloc failed for params !\n");
        hdl_source->params = params;
    }
    else {
        hdl_source->params = new_params;
    }
    hdl_source->nb_params = param_count;
    regfree(&reg);
}

void hdl_create(hdl_source_t* hdl_source) {
    memset(hdl_source, 0, sizeof(hdl_source_t));
    get_hdl_path(hdl_source->dir);
    char top_file_name[MAX_NAME_LENGTH];
    get_hdl_name(top_file_name);
    strcpy(hdl_source->top_file_path, hdl_source->dir);
    hdl_source->top_file_path[strlen(hdl_source->dir)] = '/';
    if(strlen(top_file_name) + strlen(hdl_source->dir) + 2 >= MAX_NAME_LENGTH) {
        fprintf(stderr, "Name too long !\n");
    }
    strcpy(hdl_source->top_file_path + strlen(hdl_source->dir) + 1, top_file_name);
}

void parse_hdl(hdl_source_t* hdl_source) {

    char* source = get_source(hdl_source->top_file_path, NULL);
    hdl_source->source = source;
    if(source == NULL) {
        fprintf(stderr, "The program did not manage to read the source file !\n");
    }

    get_end_of_ports_decl(hdl_source);
    get_entity_name(hdl_source);
    get_arrays(hdl_source);
    fill_arrays_ports(hdl_source);
    get_params(hdl_source);
}

void hdl_free(hdl_source_t* hdl_source) {
    if(hdl_source == NULL) {
        return;
    }
    free(hdl_source->arrays);
    free(hdl_source->params);
    free(hdl_source->source);
}
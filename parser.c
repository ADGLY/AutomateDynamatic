#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"


void get_entity_name(hdl_source_t* hdl_source, const char* source) {
    regex_t reg;
    regmatch_t match[1];
    int err = regcomp(&reg, "entity[ ]\\w+[ ]is", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, source, 1, (regmatch_t*)match, 0);
    if(err != 0) {
        fprintf(stderr, "Reg exec error !\n");
    }

    #define NAME_START_OFF 7
    char* str_match = strndup((const char*)(source + match[0].rm_so), 
        (size_t)(match[0].rm_eo - match[0].rm_so));
    char* name_start = (char*)(str_match + NAME_START_OFF);
    size_t name_len = strlen(str_match) - 10;
    strncpy(hdl_source->name, name_start, name_len);
    hdl_source->name[name_len + 1] = '\0';
    free(str_match);
    regfree(&reg);
}

void get_arrays(hdl_source_t* hdl_source, const char* source) {
    regex_t reg;
    regmatch_t match[1];
    const char* source_off = source;
    hdl_array_t* arrays = calloc(100, sizeof(hdl_array_t));
    size_t array_count = 0;

    //TODO: Put end of entity declaration and source code into hdl_source
    int err = regcomp(&reg, "end;", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }
    size_t end_of_port = match[0].rm_eo;


    err = regcomp(&reg, "\\w+_address0", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }


    for(int i = 0; i < 100; ++i) {
        err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
        if(err != 0) {
            break;
        }

        char* str_match = strndup((const char*)(source_off + match[0].rm_so), 
        (size_t)(match[0].rm_eo - match[0].rm_so));
        char* arr_name_start = str_match;
        source_off += match[0].rm_so + strlen(str_match);
        if((source_off - source) > end_of_port) {
            free(str_match);
            break;
        }

        

        size_t name_len = strlen(str_match) - 9;
        strncpy(arrays[array_count].name, arr_name_start, name_len);
        arrays[array_count].name[name_len + 1] = '\0';
        free(str_match);
        array_count++;
    }
    arrays = realloc(arrays, array_count * sizeof(hdl_array_t));
    hdl_source->arrays = arrays;
    hdl_source->nb_arrays = array_count;
    regfree(&reg);
}

void fill_interfaces(bram_interface_t* read_interface, bram_interface_t* write_interface, const char* arr_name) {

    char* pointer_to_read = (char*)read_interface;
    char* pointer_to_write = (char*)write_interface;

    for(int i = 0; i < NB_BRAM_INTERFACE; ++i) {
        size_t name_len = strlen(arr_name);
        
        //Unsafe ! Might be greater than 256 !
        strncpy(pointer_to_read, arr_name, name_len);
        strncpy(pointer_to_read + name_len, read_ports[i], strlen(read_ports[i]) + 1);

        strncpy(pointer_to_write, arr_name, name_len);
        strncpy(pointer_to_write + name_len, write_ports[i], strlen(write_ports[i]) + 1);

        pointer_to_read += MAX_NAME_LENGTH;
        pointer_to_write += MAX_NAME_LENGTH;
    }
}

void fill_arrays_ports(hdl_source_t* hdl_source) {
    for(int i = 0; i < hdl_source->nb_arrays; ++i) {
        hdl_array_t* array = &(hdl_source->arrays[i]);
        const char* arr_name = array->name;
        fill_interfaces(&(array->read_ports), &(array->write_ports), arr_name);
    }
}

void get_params(hdl_source_t* hdl_source, char* source) {
    regex_t reg;
    regmatch_t match[1];
    const char* source_off = source;
    hdl_in_param_t* params = calloc(100, sizeof(hdl_in_param_t));
    size_t param_count = 0;

    //TODO: Put end of entity declaration and source code into hdl_source
    int err = regcomp(&reg, "end;", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }
    err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
    if(err != 0) {
         fprintf(stderr, "Reg exec error !\n");
    }
    size_t end_of_port = match[0].rm_eo;


    err = regcomp(&reg, "\\w+_din ", REG_EXTENDED);
    if(err != 0) {
        fprintf(stderr, "Reg compile error !\n");
    }


    for(int i = 0; i < 100; ++i) {
        err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
        if(err != 0) {
            break;
        }

        char* str_match = strndup((const char*)(source_off + match[0].rm_so), 
        (size_t)(match[0].rm_eo - match[0].rm_so));
        char* param_name_start = str_match;
        source_off += match[0].rm_so + strlen(str_match);
        if((source_off - source) > end_of_port) {
            free(str_match);
            break;
        }

        

        size_t name_len = strlen(str_match) - 5;
        strncpy(params[param_count].name, param_name_start, name_len);
        params[param_count].name[name_len + 1] = '\0';
        free(str_match);
        param_count++;
    }
    params = realloc(params, param_count * sizeof(hdl_array_t));
    hdl_source->params = params;
    hdl_source->nb_params = param_count;
    regfree(&reg);
}

hdl_source_t* parse_hdl(char* path) {

    hdl_source_t* hdl_source = malloc(sizeof(hdl_source_t));
    strncpy(hdl_source->path, path, strlen(path) + 1);

    char* source = get_source(path);
    if(source == NULL) {
        fprintf(stderr, "THe program did not manage to read the source file !\n");
    }

    get_entity_name(hdl_source, source);
    get_arrays(hdl_source, source);
    fill_arrays_ports(hdl_source);
    get_params(hdl_source, source);
    
    return hdl_source;
}
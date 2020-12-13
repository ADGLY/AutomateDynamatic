#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define MAX_NAME_LENGTH 256


typedef struct {
    char name[MAX_NAME_LENGTH];
} hdl_in_param_t;

typedef struct {
   char address[MAX_NAME_LENGTH];
   char ce[MAX_NAME_LENGTH];
   char we[MAX_NAME_LENGTH];
   char dout[MAX_NAME_LENGTH];
   char din[MAX_NAME_LENGTH];
} bram_interface_t;

typedef struct {
    char name[MAX_NAME_LENGTH];
    bram_interface_t read_ports;
    bram_interface_t write_ports;
} hdl_array_t;


typedef struct {
    char path[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    hdl_array_t* arrays;
    hdl_in_param_t* params;
    size_t nb_arrays;
    size_t nb_params;
} hdl_source_t;

char* get_source(char* path) {
    FILE* hdl_source = fopen(path, "r");
    if(hdl_source == NULL) {
        fprintf(stderr, "Failed to open file !\n");
    }
    fseek(hdl_source, 0L, SEEK_END);
    size_t file_size = (size_t)ftell(hdl_source);
    rewind(hdl_source);
    
    
    char* source = malloc(file_size);
    fread(source, sizeof(char), file_size, hdl_source);
    return source;
}

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
            break;
        }

        

        size_t name_len = strlen(str_match) - 9;
        strncpy(arrays[array_count].name, arr_name_start, name_len);
        arrays[array_count].name[name_len + 1] = '\0';
        free(str_match);
        array_count++;
    }
    arrays = realloc(arrays, array_count * sizeof(hdl_array_t));
    regfree(&reg);
}
//histogram_elaborated_optimized.vhd
hdl_source_t* parse_hdl(char* path) {

    hdl_source_t* hdl_source = malloc(sizeof(hdl_source_t));
    strncpy(hdl_source->path, path, strlen(path) + 1);

    char* source = get_source(path);

    get_entity_name(hdl_source, source);
    get_arrays(hdl_source, source);
    
    return hdl_source;
}

void get_hdl_path(char* path) {
    printf("Where is top hdl file located ?\n");
    fgets(path, MAX_NAME_LENGTH, stdin);
    path[strlen(path) - 1] = '\0';
    return;
}

int main(void) {

    char hdl_path[MAX_NAME_LENGTH];
    get_hdl_path(hdl_path);

    hdl_source_t* source = parse_hdl(hdl_path);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include <string.h>
#include "vivado_hls.h"

error_t create_hls(vivado_hls_t* hls, hdl_source_t* hdl_source) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hdl_source);

    memset(hls, 0, sizeof(vivado_hls_t));

    char path_temp[MAX_PATH_LENGTH];
    strncpy(path_temp, hdl_source->dir, MAX_PATH_LENGTH);
    uint16_t max = (uint16_t)(MAX_PATH_LENGTH - strlen(path_temp) - 1);
    int written = snprintf(path_temp + strlen(path_temp), max, "/../src/%s.cpp", hdl_source->name);
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
    regmatch_t match[1];

    const char* source_off = hls->hls_source;
    const char* str_match;
    size_t str_match_len;

    advance_in_file_hls(&reg, match, &source_off, "return", &str_match, &str_match_len);

    advance_in_file_hls(&reg, match, &source_off, "}", &str_match, &str_match_len);

    size_t end = (size_t)(source_off - hls->hls_source) + 1;
    hls->hls_source[end] = '\0';
    char* new_source = realloc(hls->hls_source, strlen(hls->hls_source) + 1);
    CHECK_NULL(new_source, ERR_MEM, "Realloc failed on hls source !");

    source_off = hls->hls_source;

    int err = regcomp(&reg, "\\w+\\s+\\w+\\(", REG_EXTENDED);
    CHECK_COND(err != 0, ERR_REGEX, "Reg compile error !");

    err = regexec(&reg, source_off, 1, (regmatch_t*)match, 0);
    CHECK_COND_DO(err != 0, ERR_REGEX, "Reg exec error !", regfree(&reg););

    source_off = (const char*)(source_off + match[0].rm_so);

    regfree(&reg);

    advance_in_file_hls(&reg, match, &source_off, "\\w+\\(", &str_match, &str_match_len);

    strncpy(hls->fun_name, str_match, str_match_len - 1);
    hls->fun_name[str_match_len - 1] = '\0';

    //vertices((\\h)*\\[([^(\\v)])*\\])*([^(\\v)])*\\=([^(\\v)])*;

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
                CHECK_CALL(ERR_REGEX, "Wrong array type !");
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

error_t hls_free(vivado_hls_t* hls) {
    CHECK_PARAM(hls);
    CHECK_PARAM(hls->hls_source);

    free(hls->hls_source);

    return ERR_NONE;
}
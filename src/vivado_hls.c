#include <errno.h>
#include <stdio.h>
#include "vivado_hls.h"
#include "regex_wrapper.h"
#include <dirent.h>
#include <inttypes.h>
#include <string.h>

auto_error_t create_hls(vivado_hls_t *hls, hdl_info_t *hdl_info) {
    CHECK_PARAM(hls)
    CHECK_PARAM(hdl_info)

    memset(hls, 0, sizeof(vivado_hls_t));

    char path_temp[MAX_PATH_LENGTH];
    memset(path_temp, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(path_temp, hdl_info->dir, MAX_PATH_LENGTH);
    uint16_t max = (uint16_t) (MAX_PATH_LENGTH - strlen(path_temp) - 1);

    char *temp_hdl_name = strrchr(hdl_info->top_file_path, '/') + 1;
    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, MAX_NAME_LENGTH * sizeof(char));

    regmatch_t match[2];

    char hdl_name[MAX_NAME_LENGTH];
    memset(hdl_name, 0, sizeof(char) * MAX_NAME_LENGTH);
    strncpy(hdl_name, temp_hdl_name, MAX_NAME_LENGTH - 1);
    *strrchr(hdl_name, '.') = '\0';
    int written = snprintf(pattern, MAX_NAME_LENGTH, "(.*)_optimized");
    CHECK_LENGTH(written, MAX_NAME_LENGTH)


    auto_error_t err = find_pattern(pattern, hdl_name, 2, match);

    char cpp_name[MAX_NAME_LENGTH];
    memset(cpp_name, 0, MAX_NAME_LENGTH * sizeof(char));

    if (err != ERR_NONE) {
        strncpy(cpp_name, hdl_name, MAX_NAME_LENGTH);
    } else {
        strncpy(cpp_name, hdl_name, (size_t) match[1].rm_eo);
    }

    written = snprintf(path_temp + strlen(path_temp), max, "/../src/%s.cpp",
                       cpp_name);
    CHECK_LENGTH(written, max)

    char *final_source_path = realpath(path_temp, hls->source_path);
    CHECK_COND(final_source_path == NULL && errno != ENOENT, ERR_PATH,
               "Could not resolve absolute path !")

    char *final_path = realpath("vivado_hls", hls->project_path);
    CHECK_COND(final_path == NULL && errno != ENOENT, ERR_PATH,
               "Could not resolve absolute path !")

    char *source_file = read_file(hls->source_path, NULL);
    CHECK_NULL(source_file, ERR_IO, "Could not read the C/C++ source file !")
    hls->hls_source = source_file;
    return ERR_NONE;
}

auto_error_t advance_in_file_hls(regmatch_t *match, const char **source_off,
                                 const char *pattern, const char **str_match,
                                 size_t *match_len, bool modif, int nb_match) {


    auto_error_t err = find_pattern(pattern, *source_off, nb_match, match);
    if (err != ERR_NONE) {
        return err;
    }

    *str_match = *source_off + match[0].rm_so;
    *match_len = (size_t) (match[0].rm_eo - match[0].rm_so);
    if (modif) {
        *source_off = *source_off + match[0].rm_so + *match_len;
    }

    return ERR_NONE;
}

auto_error_t find_func(vivado_hls_t *hls, const char **source_off,
                       size_t *end_of_prototype, char *save, char **modif) {
    CHECK_PARAM(hls)
    CHECK_PARAM(source_off)
    CHECK_PARAM(end_of_prototype)
    CHECK_PARAM(save)
    CHECK_PARAM(modif)

    regmatch_t match[3];

    const char *str_match;
    size_t str_match_len;

    CHECK_CALL(advance_in_file_hls(
            match, source_off,
            "\\w+\\s+(\\w+)\\s*(\\()[^(\\))]*\\)[[:space:]]*\\{",
            &str_match, &str_match_len, false, 3),
               "advance_in_file_hls failed !")

    size_t sub_match_len = (size_t) (match[1].rm_eo - match[1].rm_so);
    CHECK_COND(sub_match_len >= MAX_NAME_LENGTH, ERR_REGEX, "Name too long !")
    strncpy(hls->fun_name, *source_off + match[1].rm_so, sub_match_len);
    hls->fun_name[sub_match_len] = '\0';
    *source_off = *source_off + match[2].rm_eo;

    *end_of_prototype = (size_t) (match[0].rm_eo - match[2].rm_eo);

    CHECK_CALL(advance_in_file_hls(
            match, source_off,
            "\\w+\\s+(\\w+)\\s*(\\()[^(\\))]*\\)[[:space:]]*\\{",
            &str_match, &str_match_len, false, 1),
               "advance_in_file_hls failed !")

    *save = (*source_off)[match[0].rm_so - 1];
    *modif = (char *) (&((*source_off)[match[0].rm_so - 1]));
    *(*modif) = '\0';

    return ERR_NONE;
}

bool check_type(const char *str_match, hdl_array_t *arr) {
    if (str_match == NULL || arr == NULL) {
        fprintf(stderr, "An arg of a \"private\" function is NULL, this should "
                        "not happen.");
        return false;
    }

    if (strncmp("in_", str_match, 3) == 0) {
        arr->read = true;
        return true;
    }
    if (strncmp("inout_", str_match, 6) == 0) {
        arr->read = true;
        arr->write = true;
        return true;
    }
    if (strncmp("out_", str_match, 4) == 0) {
        arr->write = true;
        return true;
    }
    return false;
}

typedef enum {
    READ, WRITE
} check_mode_t;

bool check_square_bracket(const char *look_for_arrays, hdl_array_t *arr,
                          check_mode_t mode) {
    if (look_for_arrays == NULL || arr == NULL) {
        fprintf(stderr, "An arg of a \"private\" function is NULL, this should "
                        "not happen.");
        return false;
    }

    regmatch_t match[1];
    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, MAX_NAME_LENGTH * sizeof(char));
    if (mode == READ) {
        int written =
                snprintf(pattern, MAX_NAME_LENGTH,
                         "%s[[:space:]]*((\\[)[^]]*(\\]))+([^=]*==[^=]*)*[^=;]*;",
                         arr->name);
        CHECK_LENGTH(written, MAX_NAME_LENGTH)
    } else {
        int written = snprintf(
                pattern, MAX_NAME_LENGTH,
                "%s[[:space:]]*((\\[)[^]]*(\\]))+([^=]*==[^=]*)*[^=;]*=[^;]*;",
                arr->name);
        CHECK_LENGTH(written, MAX_NAME_LENGTH)
    }

    auto_error_t err = find_pattern(pattern, look_for_arrays, 1, match);
    if (err == ERR_NONE) {
        if (mode == READ) {
            arr->read = true;
        } else {
            arr->write = true;
        }
        return true;
    }
    return false;
}

bool check_parentheses(const char *look_for_arrays, hdl_array_t *arr,
                       check_mode_t mode) {
    if (look_for_arrays == NULL || arr == NULL) {
        fprintf(stderr,
                "Arg for \"private\" func is NULL, this should not happen !");
        return false;
    }

    regmatch_t match[2];
    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, MAX_NAME_LENGTH * sizeof(char));

    if (mode == READ) {
        int written =
                snprintf(pattern, MAX_NAME_LENGTH,
                         "[*](\\((?:[^)(]+|(?1))*+\\))([^=]*==[^=]*)*[^=;]*;");
        CHECK_LENGTH(written, MAX_NAME_LENGTH)
    } else {
        int written = snprintf(
                pattern, MAX_NAME_LENGTH,
                "[*](\\((?:[^)(]+|(?1))*+\\))([^=]*==[^=]*)*[^=;]*=[^;]*;");
        CHECK_LENGTH(written, MAX_NAME_LENGTH)
    }

    set_pattern(pattern, 1);
    const char *offset = look_for_arrays;
    auto_error_t  err;
    do {
        err = find_set_pattern(offset, 2, match, 1);
        if (err == ERR_NONE) {
            const char *match_start = offset + match[1].rm_so;
            size_t match_len = (size_t) (match[1].rm_eo - match[1].rm_so);
            CHECK_COND(match_len >= MAX_NAME_LENGTH, ERR_REGEX,
                          "Name too long !")

            char str_match[MAX_NAME_LENGTH];
            memset(str_match, 0, MAX_NAME_LENGTH * sizeof(char));
            strncpy(str_match, match_start, match_len);
            str_match[match_len] = '\0';
            char *result = strstr(str_match, arr->name);
            if (result != NULL) {
                if (mode == READ) {
                    arr->read = true;
                } else {
                    arr->write = true;
                }
                return true;
            }
            offset += match[0].rm_eo;
        }
    } while (err == ERR_NONE);
    return false;
}

auto_error_t determine_read_or_write(hdl_info_t *hdl_info,
                                     const char *source_off,
                                     size_t end_of_prototype) {

    CHECK_PARAM(hdl_info)
    CHECK_PARAM(source_off)

    regmatch_t match[1];
    const char *str_match;
    size_t str_match_len;

    for (size_t i = 0; i < hdl_info->nb_arrays; ++i) {
        const char *name = hdl_info->arrays[i].name;
        char pattern[MAX_NAME_LENGTH];
        memset(pattern, 0, MAX_NAME_LENGTH);
        int written =
                snprintf(pattern, MAX_NAME_LENGTH, "\\w+(\\s)*%s(\\s)*\\[", name);
        CHECK_LENGTH(written, MAX_NAME_LENGTH)

        CHECK_CALL(advance_in_file_hls(match, &source_off, pattern, &str_match,
                                       &str_match_len, false, 1),
                   "advance_in_file_hls failed !")

        str_match = source_off + match[0].rm_so;
        str_match_len = (size_t) (match[0].rm_eo - match[0].rm_so);

        hdl_info->arrays[i].read = false;
        hdl_info->arrays[i].write = false;

        bool res = check_type(str_match, &(hdl_info->arrays[i]));

        if (!res) {
            // Read matching
            const char *look_for_arrays = source_off + end_of_prototype;
            uint8_t nb_match = 0;
            res = check_square_bracket(look_for_arrays,
                                       &(hdl_info->arrays[i]), READ);
            if (!res) {
                res = check_parentheses(look_for_arrays,
                                        &(hdl_info->arrays[i]), READ);
                if (!res) {
                    nb_match++;
                }
            }
            // Write matching
            res = check_square_bracket(look_for_arrays,
                                       &(hdl_info->arrays[i]), WRITE);
            if (!res) {
                res = check_parentheses(look_for_arrays,
                                        &(hdl_info->arrays[i]), WRITE);
                if (!res) {
                    if (nb_match == 1) {
                        fprintf(
                                stderr,
                                "Array is found to be neither read nor write !");
                        return ERR_REGEX;
                    }
                }
            }
        }
    }
    return ERR_NONE;
}

auto_error_t parse_hls(vivado_hls_t *hls, hdl_info_t *hdl_info) {
    CHECK_PARAM(hls)
    CHECK_PARAM(hls->hls_source)
    CHECK_PARAM(hdl_info)
    CHECK_PARAM(hdl_info->arrays)

    const char *source_off;

    char *new_source = realloc(hls->hls_source, strlen(hls->hls_source) + 1);
    CHECK_NULL(new_source, ERR_MEM, "Realloc failed on hls source !")

    hls->hls_source = new_source;

    source_off = hls->hls_source;

    size_t end_prototype;
    char save;
    char *modif;
    CHECK_CALL(find_func(hls, &source_off, &end_prototype, &save, &modif),
               "find_func failed !")
    CHECK_CALL(determine_read_or_write(hdl_info, source_off, end_prototype),
               "determine_read_or_write failed !")
    *modif = save;
    return ERR_NONE;
}

auto_error_t launch_hls_script() {

    FILE *vivado_hls_input;
    vivado_hls_input = popen("vivado_hls -f hls_script.tcl", "w");
    CHECK_COND(vivado_hls_input == NULL, ERR_IO, "Failed to launch Vivado !")
    fprintf(vivado_hls_input, "exit\n");
    pclose(vivado_hls_input);
    return ERR_NONE;
}

typedef enum {
    HDL, TCL
} file_type_t;

float_op_t *check_op_already_exists(uint8_t count, const char *op_name,
                                    float_op_t *float_op) {
    for (uint8_t i = 0; i < count; ++i) {
        float_op_t *op = &(float_op[i]);
        if (strcmp(op_name, op->name) == 0) {
            return op;
        }
    }
    return NULL;
}

auto_error_t resolve_op_name(char *op_name, const char *path,
                             const char *fun_name) {
    CHECK_PARAM(path)

    char *fop_file = read_file(path, NULL);
    CHECK_NULL(fop_file, ERR_IO, "Could not read fop file !")

    regmatch_t match[2];

    auto_error_t  err = find_pattern("component[[:space:]]*(\\w+)[[:space:]]*is", fop_file, 2, match);

    CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Reg exec error !", free(fop_file);)

    char comp_name[MAX_NAME_LENGTH];
    memset(comp_name, 0, sizeof(char) * MAX_NAME_LENGTH);

    size_t name_len = (size_t) (match[1].rm_eo - match[1].rm_so);
    strncpy(comp_name, fop_file + match[1].rm_so, name_len);

    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, MAX_NAME_LENGTH * sizeof(char));
    int written = snprintf(pattern, MAX_NAME_LENGTH,
                           "%s_[[:alnum:]]*_([[:alnum:]]*)", fun_name);
    CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                  "Name too long !", free(fop_file);)

    err = find_pattern(pattern, comp_name, 2, match);
    CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Reg exec error !", free(fop_file);)

    strncpy(op_name, comp_name + match[1].rm_so,
            (size_t) (match[1].rm_eo - match[1].rm_so));

    free(fop_file);

    return ERR_NONE;
}

auto_error_t check_file(uint8_t *count, float_op_t *floats,
                        const char *float_paths, mode_t mode,
                        const char *fun_name, const char *file_name,
                        bool *faddfsub) {

    CHECK_PARAM(count)
    CHECK_PARAM(floats)
    CHECK_PARAM(float_paths)

    char file_path[MAX_PATH_LENGTH];
    memset(file_path, 0, sizeof(char) * MAX_PATH_LENGTH);

    strncpy(file_path, float_paths, MAX_PATH_LENGTH - 1);
    size_t len = strlen(file_path);
    CHECK_LENGTH(len, MAX_PATH_LENGTH - 1)
    file_path[len] = '/';
    file_path[len + 1] = '\0';
    uint16_t max = (uint16_t) (MAX_PATH_LENGTH - strlen(file_path));
    strncpy(file_path + strlen(file_path), file_name, max);

    char op_name[MAX_NAME_LENGTH];
    memset(op_name, 0, sizeof(char) * MAX_NAME_LENGTH);
    // We can't use the name of the hdl file because on names greater than 20
    // they are renamed We need to look at the component name inside the vhdl
    // file
    if (mode == HDL) {
        CHECK_CALL(resolve_op_name(op_name, file_path, fun_name),
                   "resolve_op_name failed !")
    } else {
        regmatch_t match[2];

        char pattern[MAX_NAME_LENGTH];
        memset(pattern, 0, MAX_NAME_LENGTH * sizeof(char));

        int written = snprintf(pattern, MAX_NAME_LENGTH,
                               "%s_[[:alnum:]]*_([[:alnum:]]*)", fun_name);
        CHECK_LENGTH(written, MAX_NAME_LENGTH)

        auto_error_t err = find_pattern(pattern, file_name, 2, match);
        CHECK_COND(err != ERR_NONE, ERR_REGEX, "Regex exec failed !")

        size_t name_len = (size_t) (match[1].rm_eo - match[1].rm_so);
        CHECK_COND(name_len >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                   "Name too long !")

        strncpy(op_name, file_name + match[1].rm_so, name_len);

        if (strstr(op_name, "faddfsub") != NULL) {
            *faddfsub = true;
        }
    }

    // Check if it exists
    char *path_to_modify;
    float_op_t *op = check_op_already_exists(*count, op_name, floats);
    if (op == NULL) {
        op = &(floats[*count]);
        strncpy(op->name, op_name, MAX_NAME_LENGTH);
        *count = (uint8_t) (*count + 1);
    }
    if (mode == HDL) {
        path_to_modify = op->hdl_path;
    } else {
        path_to_modify = op->script_path;
    }
    strncpy(path_to_modify, file_path, MAX_PATH_LENGTH);
    return ERR_NONE;
}

float_op_t *find_faddfsub(vivado_hls_t *hls) {
    float_op_t *op = NULL;
    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        op = &(hls->float_ops[i]);
        if (strstr(op->name, "faddfsub") != NULL) {
            break;
        }
    }
    return op;
}

void string_manip_tcl_faddfsub(float_op_t *faddfsub, char *filepath,
                               char *old_filename, char *new_filename_fadd,
                               char *fadd_path) {

    char *last_slash = strrchr(faddfsub->script_path, '/');
    if (last_slash == NULL) {
        last_slash = faddfsub->script_path;
    } else {
        last_slash++;
    }

    memset(filepath, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(filepath, faddfsub->script_path,
            (size_t) (last_slash - faddfsub->script_path));

    memset(old_filename, 0, MAX_NAME_LENGTH * sizeof(char));
    strncpy(old_filename, last_slash, MAX_NAME_LENGTH);

    char *substr = strstr(faddfsub->script_path, "faddfsub");
    memset(new_filename_fadd, 0, sizeof(char) * MAX_NAME_LENGTH);
    strncpy(new_filename_fadd, last_slash, (size_t) (substr - last_slash));
    strncpy(new_filename_fadd + strlen(new_filename_fadd), substr, 4);
    strncpy(new_filename_fadd + strlen(new_filename_fadd), substr + 8,
            MAX_NAME_LENGTH);

    memset(fadd_path, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(fadd_path, filepath, MAX_PATH_LENGTH);
    strncpy(fadd_path + strlen(fadd_path), new_filename_fadd,
            strlen(new_filename_fadd));
}

void update_tcl_fadd(char *fadd_path, char *new_filename_fadd,
                     char *old_filename_fadd, char *component_name_add) {
    char *tcl_script = read_file(fadd_path, NULL);

    const char *tcl_script_off = tcl_script;

    memset(component_name_add, 0, MAX_NAME_LENGTH * sizeof(char));

    strncpy(component_name_add, new_filename_fadd, MAX_NAME_LENGTH);

    *strrchr(component_name_add, '_') = '\0';

    FILE *tcl_script_file = fopen(fadd_path, "w");
    regmatch_t match[1];

    *strrchr(old_filename_fadd, '_') = '\0';

    const char *str_match;
    size_t match_len;

    //------------------------------------------------------------------
    advance_in_file_hls(match, &tcl_script_off, old_filename_fadd, &str_match,
                        &match_len, false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo + 1;

    fprintf(tcl_script_file, "%s\n", component_name_add);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &tcl_script_off, "Both", &str_match, &match_len,
                        false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo + 1;

    fprintf(tcl_script_file, "Add ");
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &tcl_script_off, old_filename_fadd, &str_match,
                        &match_len, false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo + 1;

    fprintf(tcl_script_file, "%s ", component_name_add);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &tcl_script_off, old_filename_fadd, &str_match,
                        &match_len, false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;

    fprintf(tcl_script_file, "%s", component_name_add);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &tcl_script_off, old_filename_fadd, &str_match,
                        &match_len, false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;

    fprintf(tcl_script_file, "%s", component_name_add);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &tcl_script_off, old_filename_fadd, &str_match,
                        &match_len, false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;

    fprintf(tcl_script_file, "%s", component_name_add);
    //------------------------------------------------------------------

    fwrite(tcl_script_off, sizeof(char), strlen(tcl_script_off),
           tcl_script_file);

    fclose(tcl_script_file);

    free((void *) tcl_script);
}

float_op_t *find_fadd(vivado_hls_t *hls) {
    float_op_t *op_add = NULL;
    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        op_add = &(hls->float_ops[i]);
        if (strstr(op_add->name, "faddfsub") != NULL) {
            break;
        }
    }
    return op_add;
}

void string_manip_hdl_faddfsub(vivado_hls_t *hls, char *new_hdl_name,
                               char *old_hdl_name, char *new_hdl_path) {
    float_op_t *old_op_add = find_fadd(hls);
    memset(new_hdl_name, 0, sizeof(char) * MAX_NAME_LENGTH);
    memset(old_hdl_name, 0, sizeof(char) * MAX_NAME_LENGTH);

    char *last_slash = strrchr(old_op_add->hdl_path, '/');
    strncpy(old_hdl_name, last_slash + 1, strlen(last_slash) - 1);

    strncpy(new_hdl_name, old_hdl_name,
            (size_t) (strstr(old_hdl_name, "fadd") - old_hdl_name));
    strncat(new_hdl_name, "fadd.vhd", MAX_NAME_LENGTH);
    memset(new_hdl_path, 0, sizeof(char) * MAX_NAME_LENGTH);

    strncpy(new_hdl_path, old_op_add->hdl_path,
            (size_t) (last_slash - old_op_add->hdl_path) + 1);
    strncpy(new_hdl_path + strlen(new_hdl_path), new_hdl_name,
            strlen(new_hdl_name));
}

void update_hdl_fadd(float_op_t *fadd, char *old_hdl_name, char *new_hdl_name,
                     char *old_filename_fadd, char *component_name_add) {
    char *hdl_fadd = read_file(fadd->hdl_path, NULL);
    const char *hdl_off = hdl_fadd;
    FILE *hdl_fadd_file = fopen(fadd->hdl_path, "w");

    regmatch_t match[1];
    const char *str_match;
    size_t match_len;
    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off, old_hdl_name, &str_match, &match_len,
                        false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;

    fprintf(hdl_fadd_file, "%s ", new_hdl_name);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(
            match, &hdl_off,
            "opcode\\s+\\:\\s+in\\s+std_logic_vector\\(1 downto 0\\);", &str_match,
            &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off, old_hdl_name, &str_match, &match_len,
                        false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;

    fprintf(hdl_fadd_file, "%s ", new_hdl_name);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off, old_filename_fadd, &str_match,
                        &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;

    fprintf(hdl_fadd_file, "%s ", component_name_add);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off,
                        "s_axis_operation_tvalid\\s+\\:\\s+in\\s+std_logic;",
                        &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off,
                        "s_axis_operation_tdata\\s+\\:\\s+in\\s+std_logic_"
                        "vector\\(7\\s+downto\\s+0\\);",
                        &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off,
                        "signal\\s+op_tvalid\\s+\\:\\s+std_logic;", &str_match,
                        &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(
            match, &hdl_off,
            "signal\\s+op_tdata\\s+\\:\\s+std_logic_vector\\(7\\s+downto\\s+0\\);",
            &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off,
                        "signal\\s+opcode_buf1\\s+\\:\\s+std_logic_vector\\("
                        "1\\s+downto\\s+0\\);",
                        &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    char pattern[MAX_NAME_LENGTH];
    snprintf(pattern, MAX_NAME_LENGTH, "component\\s+%s", old_filename_fadd);
    advance_in_file_hls(match, &hdl_off, pattern, &str_match, &match_len, false,
                        1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;

    fprintf(hdl_fadd_file, "component %s \n", component_name_add);
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off,
                        "s_axis_operation_tvalid\\s+=>\\s+op_tvalid,",
                        &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off,
                        "s_axis_operation_tdata\\s+=>\\s+op_tdata,", &str_match,
                        &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off, "op_tvalid\\s+<=\\s+'1';", &str_match,
                        &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(
            match, &hdl_off,
            "op_tdata\\s+<=\\s+\\(\\s+\"000000\"\\s+&\\s+opcode_buf1\\);",
            &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    advance_in_file_hls(match, &hdl_off, "opcode_buf1\\s+<=\\s+opcode;",
                        &str_match, &match_len, false, 1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_fadd_file);
    hdl_off += match[0].rm_eo + 1;
    //------------------------------------------------------------------
    fwrite(hdl_off, sizeof(char), strlen(hdl_off), hdl_fadd_file);
    fclose(hdl_fadd_file);
    free((void *) hdl_fadd);
}

float_op_t *create_fadd(vivado_hls_t *hls) {

    float_op_t *faddfsub = find_faddfsub(hls);

    char filepath[MAX_PATH_LENGTH];
    char old_filename_fadd[MAX_PATH_LENGTH];
    char new_filename_fadd[MAX_PATH_LENGTH];
    char fadd_path[MAX_PATH_LENGTH];

    string_manip_tcl_faddfsub(faddfsub, filepath, old_filename_fadd,
                              new_filename_fadd, fadd_path);

    rename(faddfsub->script_path, fadd_path);

    char component_name_add[MAX_NAME_LENGTH];
    update_tcl_fadd(fadd_path, new_filename_fadd, old_filename_fadd,
                    component_name_add);

    float_op_t *new_fadd_op = calloc(1, sizeof(float_op_t));

    memset(new_fadd_op->name, 0, MAX_NAME_LENGTH * sizeof(char));
    strncpy(new_fadd_op->name, "fadd", MAX_NAME_LENGTH);

    memset(new_fadd_op->script_path, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(new_fadd_op->script_path, fadd_path, MAX_PATH_LENGTH);

    char new_hdl_name[MAX_NAME_LENGTH];
    char old_hdl_name[MAX_NAME_LENGTH];
    char new_hdl_path[MAX_NAME_LENGTH];

    string_manip_hdl_faddfsub(hls, new_hdl_name, old_hdl_name, new_hdl_path);

    rename(faddfsub->hdl_path, new_hdl_path);

    memset(new_fadd_op->hdl_path, 0, sizeof(char) * MAX_PATH_LENGTH);

    strncpy(new_fadd_op->hdl_path, new_hdl_path, MAX_PATH_LENGTH);

    *strrchr(old_hdl_name, '.') = '\0';
    *strrchr(new_hdl_name, '.') = '\0';

    update_hdl_fadd(new_fadd_op, old_hdl_name, new_hdl_name, old_filename_fadd,
                    component_name_add);

    return new_fadd_op;
}

float_op_t *create_fsub(float_op_t *fadd_op) {

    char tcl_script_path[MAX_PATH_LENGTH];
    memset(tcl_script_path, 0, sizeof(char) * MAX_PATH_LENGTH);

    char *name = strrchr(fadd_op->script_path, '/') + 1;

    strncpy(tcl_script_path, fadd_op->script_path,
            (size_t) (name - fadd_op->script_path));

    char tcl_script_name[MAX_NAME_LENGTH];
    memset(tcl_script_name, 0, sizeof(char) * MAX_NAME_LENGTH);

    strncpy(tcl_script_name, name, (size_t) (strstr(name, "fadd") - name));
    snprintf(tcl_script_name + strlen(tcl_script_name), MAX_NAME_LENGTH,
             "fsub%s", strstr(name, "fadd") + 4);

    strncat(tcl_script_path, tcl_script_name, strlen(tcl_script_name));

    FILE *tcl_script_file = fopen(tcl_script_path, "w");

    char *tcl_script_fadd = read_file(fadd_op->script_path, NULL);
    const char *tcl_script_off = tcl_script_fadd;

    regmatch_t match[1];
    const char *str_match;
    size_t match_len;

    //------------------------------------------------------------------

    advance_in_file_hls(match, &tcl_script_off, "fadd", &str_match, &match_len,
                        false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;
    fprintf(tcl_script_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &tcl_script_off, "CONFIG\\.add_sub_value\\s+Add",
                        &str_match, &match_len, false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;
    fprintf(tcl_script_file, "CONFIG.add_sub_value Subtract");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &tcl_script_off, "fadd", &str_match, &match_len,
                        false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;
    fprintf(tcl_script_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &tcl_script_off, "fadd", &str_match, &match_len,
                        false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;
    fprintf(tcl_script_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &tcl_script_off, "fadd", &str_match, &match_len,
                        false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;
    fprintf(tcl_script_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &tcl_script_off, "fadd", &str_match, &match_len,
                        false, 1);

    fwrite(tcl_script_off, sizeof(char), (size_t) (match[0].rm_so),
           tcl_script_file);
    tcl_script_off += match[0].rm_eo;
    fprintf(tcl_script_file, "fsub");
    //------------------------------------------------------------------

    fwrite(tcl_script_off, sizeof(char), strlen(tcl_script_off),
           tcl_script_file);

    fclose(tcl_script_file);
    free(tcl_script_fadd);

    char hdl_path[MAX_PATH_LENGTH];
    memset(hdl_path, 0, sizeof(char) * MAX_PATH_LENGTH);

    name = strrchr(fadd_op->hdl_path, '/') + 1;

    strncpy(hdl_path, fadd_op->hdl_path, (size_t) (name - fadd_op->hdl_path));

    char hdl_name[MAX_NAME_LENGTH];
    memset(hdl_name, 0, sizeof(char) * MAX_NAME_LENGTH);

    strncpy(hdl_name, name, (size_t) (strstr(name, "fadd") - name));
    snprintf(hdl_name + strlen(hdl_name), MAX_NAME_LENGTH, "fsub%s",
             strstr(name, "fadd") + 4);

    strncat(hdl_path, hdl_name, strlen(hdl_name));

    FILE *hdl_file = fopen(hdl_path, "w");

    char *hdl_fadd = read_file(fadd_op->hdl_path, NULL);
    const char *hdl_off = hdl_fadd;

    //------------------------------------------------------------------

    advance_in_file_hls(match, &hdl_off, "fadd", &str_match, &match_len, false,
                        1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_file);
    hdl_off += match[0].rm_eo;
    fprintf(hdl_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &hdl_off, "fadd", &str_match, &match_len, false,
                        1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_file);
    hdl_off += match[0].rm_eo;
    fprintf(hdl_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &hdl_off, "fadd", &str_match, &match_len, false,
                        1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_file);
    hdl_off += match[0].rm_eo;
    fprintf(hdl_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &hdl_off, "fadd", &str_match, &match_len, false,
                        1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_file);
    hdl_off += match[0].rm_eo;
    fprintf(hdl_file, "fsub");
    //------------------------------------------------------------------

    //------------------------------------------------------------------

    advance_in_file_hls(match, &hdl_off, "fadd", &str_match, &match_len, false,
                        1);

    fwrite(hdl_off, sizeof(char), (size_t) (match[0].rm_so), hdl_file);
    hdl_off += match[0].rm_eo;
    fprintf(hdl_file, "fsub");
    //------------------------------------------------------------------

    fwrite(hdl_off, sizeof(char), strlen(hdl_off), hdl_file);

    fclose(hdl_file);
    free(hdl_fadd);

    float_op_t *new_fsub_op = calloc(1, sizeof(float_op_t));
    memset(new_fsub_op, 0, sizeof(float_op_t));

    strncpy(new_fsub_op->hdl_path, hdl_path, MAX_PATH_LENGTH);
    strncpy(new_fsub_op->script_path, tcl_script_path, MAX_PATH_LENGTH);
    strncpy(new_fsub_op->name, "fsub", MAX_NAME_LENGTH);

    return new_fsub_op;
}

auto_error_t handle_faddfsub(vivado_hls_t *hls) {

    float_op_t *fadd_op = create_fadd(hls);
    float_op_t *fsub_op = create_fsub(fadd_op);

    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t *op = &(hls->float_ops[i]);
        if (strstr(op->name, "faddfsub") != NULL) {
            memset(op, 0, sizeof(float_op_t));
            strcpy(op->name, fadd_op->name);
            strcpy(op->script_path, fadd_op->script_path);
            strcpy(op->hdl_path, fadd_op->hdl_path);
        }
    }

    float_op_t *new_floats = realloc(
            hls->float_ops, (size_t) (sizeof(float_op_t) * (size_t) (hls->nb_float_op + 1)));
    if (new_floats == NULL) {
        return ERR_MEM;
    }

    hls->float_ops = new_floats;

    float_op_t *op = &(new_floats[hls->nb_float_op]);
    memset(op, 0, sizeof(float_op_t));
    strcpy(op->name, fsub_op->name);
    strcpy(op->script_path, fsub_op->script_path);
    strcpy(op->hdl_path, fsub_op->hdl_path);
    hls->nb_float_op = (uint8_t) (hls->nb_float_op + 1);
    free(fadd_op);
    free(fsub_op);

    return ERR_NONE;
}

auto_error_t update_float_op(const char *float_paths, vivado_hls_t *hls) {

    CHECK_PARAM(float_paths)
    CHECK_PARAM(hls)

    char hdl_name[MAX_NAME_LENGTH];
    memset(hdl_name, 0, MAX_NAME_LENGTH * sizeof(char));
    strncpy(hdl_name, hls->fun_name, MAX_NAME_LENGTH);
    uint16_t max = (uint16_t) (MAX_NAME_LENGTH - strlen(hdl_name));
    strncat(hdl_name, ".vhd", max);

    DIR *d;
    d = opendir(float_paths);
    CHECK_COND(d == NULL, ERR_IO, "Could not open dir !")

    uint8_t count = 0;
    float_op_t *floats = calloc(5, sizeof(float_op_t));
    CHECK_COND_DO(floats == NULL, ERR_MEM,
                  "Could not allocate space for float ops !", closedir(d);)
    uint8_t last = 5;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (count == last) {
            float_op_t *new_float_ops = realloc(floats, (size_t) (last * 2));
            CHECK_COND_DO(new_float_ops == NULL, ERR_MEM,
                          "Could not reallocate for float ops !", closedir(d);
                                  free(floats);)
            floats = new_float_ops;
            last = (uint8_t) (last * 2);
        }
        if (strncmp(hdl_name, dir->d_name, MAX_NAME_LENGTH) != 0) {
            char *file_ext = strrchr(dir->d_name, '.');
            CHECK_COND(file_ext == NULL, ERR_IO, "No file ext !")
            if (strncmp(file_ext + 1, "vhd", 3) == 0) {
                CHECK_CALL_DO(check_file(&count, floats, float_paths, HDL,
                                         hls->fun_name, dir->d_name,
                                         &(hls->faddfsub)),
                              "check_file failed !", closedir(d);
                                      free(floats);)
            } else if (strncmp(file_ext + 1, "tcl", 3) == 0) {
                CHECK_CALL_DO(check_file(&count, floats, float_paths, TCL,
                                         hls->fun_name, dir->d_name,
                                         &(hls->faddfsub)),
                              "check_file failed !", closedir(d);
                                      free(floats);)
            }
        }
    }

    closedir(d);
    hls->float_ops = floats;
    hls->nb_float_op = count;
    if (hls->faddfsub) {
        handle_faddfsub(hls);
    }
    return ERR_NONE;
}

auto_error_t find_float_op(vivado_hls_t *hls) {
    CHECK_PARAM(hls)

    char float_paths[MAX_PATH_LENGTH];
    memset(float_paths, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(float_paths, hls->project_path, MAX_PATH_LENGTH);
    uint16_t max = (uint16_t) (MAX_PATH_LENGTH - strlen(float_paths) - 1);
    int written =
            snprintf(float_paths + strlen(float_paths), max, "/solution1/syn/vhdl");
    CHECK_LENGTH(written, max)

    update_float_op(float_paths, hls);

    return ERR_NONE;
}

auto_error_t open_dot_file(vivado_hls_t *hls, hdl_info_t *hdl) {
    CHECK_PARAM(hls)
    CHECK_PARAM(hdl)

    char dot_file_name[MAX_NAME_LENGTH];
    memset(dot_file_name, 0, sizeof(char) * MAX_NAME_LENGTH);
    strncpy(dot_file_name, hdl->top_file_name, MAX_NAME_LENGTH);
    *strrchr(dot_file_name, '.') = '\0';
    strncat(dot_file_name, ".dot", MAX_NAME_LENGTH - 1);

    char dot_file_path[MAX_PATH_LENGTH];
    memset(dot_file_path, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(dot_file_path, hdl->dir, MAX_PATH_LENGTH);

    uint16_t max = (uint16_t) (MAX_PATH_LENGTH - strlen(dot_file_path));
    CHECK_COND(max > MAX_PATH_LENGTH, ERR_NAME_TOO_LONG, "Name too long !")
    strncat(dot_file_path, "/../reports/", max);

    max = (uint16_t) (MAX_PATH_LENGTH - strlen(dot_file_path));
    CHECK_COND(max > MAX_PATH_LENGTH, ERR_NAME_TOO_LONG, "Name too long !")
    strncat(dot_file_path, dot_file_name, max);

    char *dot_source = read_file(dot_file_path, NULL);
    CHECK_NULL(dot_source, ERR_IO, "Couldn't read dot_source")

    regmatch_t match[3];
    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, sizeof(char) * MAX_NAME_LENGTH);
    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t *float_op = &(hls->float_ops[i]);
        char *dot_source_off = dot_source;
        auto_error_t err = ERR_NONE;

        if (strncmp(float_op->name, "fcmp", 4) == 0) {
            float_op->latency = 0;
        } else {
            int written = snprintf(pattern, MAX_NAME_LENGTH,
                                   "%s([^(\\v)])*latency=", float_op->name);
            CHECK_COND_DO(written >= MAX_PATH_LENGTH, ERR_NAME_TOO_LONG,
                          "Name too long !", free(dot_source);)


            err = find_pattern(pattern, dot_source, 1, match);
            CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Regex exec failed !",
                          free(dot_source);)

            char *start_match = dot_source + match[0].rm_eo;
            uint8_t dot_latency = (uint8_t) strtol(start_match, NULL, 10);
            float_op->latency = (uint8_t) (dot_latency - 2);
        }
        char **name_list = NULL;
        uint8_t last = 0;
        allocate_str_arr(&name_list, &last);
        uint8_t nb_names = 0;

        int written = snprintf(pattern, MAX_NAME_LENGTH,
                               "%s([^(\\v)])*op = \"(\\w+)\"", float_op->name);
        CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                      "Name too long !", free(dot_source);
                              free_str_arr(name_list, last);)
        set_pattern(pattern, 1);
        CHECK_COND_DO(err != 0, ERR_REGEX, "Regex compilation failed !",
                      free(dot_source);
                              free_str_arr(name_list, last);)
        while (err == 0) {
            err = find_set_pattern(dot_source_off, 3, match, 1);
            if (err != 0) {
                break;
            } else {
                strncpy(name_list[nb_names], dot_source_off + match[2].rm_so,
                        (size_t) (match[2].rm_eo - match[2].rm_so));
                nb_names++;
                dot_source_off += match[0].rm_eo;
            }
        }
        CHECK_COND_DO(nb_names == 0, ERR_IO, "At least one name is needed !",
                      free(dot_source);
                              free_str_arr(name_list, last);)
        for (uint8_t j = nb_names; j < last; ++j) {
            free(name_list[j]);
        }

        char **new_name_list = realloc(name_list, nb_names * sizeof(char *));
        if (new_name_list == NULL) {
            return ERR_MEM;
        }
        float_op->arith_unit_name_list = new_name_list;
        float_op->nb_arith_names = nb_names;
    }
    free(dot_source);

    return ERR_NONE;
}

int compare(const void *elem1, const void *elem2) {
    const float_op_t *op1 = *((const float_op_t **) elem1);
    const float_op_t *op2 = *((const float_op_t **) elem2);
    if (op1->appearance_offset > op2->appearance_offset) {
        return 1;
    }
    if (op1->appearance_offset < op2->appearance_offset) {
        return -1;
    }
    return 0;
}

auto_error_t update_arithmetic_units(project_t *project, vivado_hls_t *hls,
                                     axi_ip_t *axi_ip) {
    CHECK_PARAM(project)
    CHECK_PARAM(hls)

    char arithmetic_path[MAX_PATH_LENGTH];
    memset(arithmetic_path, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(arithmetic_path, axi_ip->path, MAX_PATH_LENGTH);
    int written =
            snprintf(arithmetic_path + strlen(arithmetic_path), MAX_NAME_LENGTH,
                     "/%s_1.0/src/arithmetic_units.vhd", axi_ip->name);
    CHECK_LENGTH(written, MAX_NAME_LENGTH)

    char *arithmetic_source = read_file(arithmetic_path, NULL);
    CHECK_NULL(arithmetic_source, ERR_IO, "Could not read arithmetic source !")
    char *source_off = arithmetic_source;

    float_op_t **sorted_by_appearance =
            calloc(hls->nb_float_op, sizeof(float_op_t *));
    CHECK_COND_DO(sorted_by_appearance == NULL, ERR_MEM,
                  "Could not allocate memory !", free(arithmetic_source);)

    uint8_t count = 0;
    char pattern[MAX_NAME_LENGTH];
    memset(pattern, 0, sizeof(char) * MAX_NAME_LENGTH);
    regmatch_t match[2];

    // WARNING: The order should be valid because arith names are put in order
    // in the list
    // TODO: Make it more reliable
    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t *op = &(hls->float_ops[i]);
        snprintf(pattern, MAX_NAME_LENGTH, "%s", op->name);

        auto_error_t err = find_pattern(pattern, source_off, 1, match);
        CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Regex exec failed !",
                free(arithmetic_source); free(sorted_by_appearance);)

        op->appearance_offset = (size_t) match[0].rm_so;
        sorted_by_appearance[count++] = op;
        qsort(sorted_by_appearance, count, sizeof(float_op_t *), compare);
    }

    FILE *arithmetic_file = fopen(arithmetic_path, "w");
    CHECK_COND_DO(arithmetic_path == NULL, ERR_IO,
                  "Could not open file arithmetic_units.vhd !",
                  free(arithmetic_source);
                          free(sorted_by_appearance);)

    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t *op = sorted_by_appearance[i];
        for (size_t j = 0; j < op->nb_arith_names; ++j) {
            char *name = op->arith_unit_name_list[j];
            written = snprintf(pattern, MAX_NAME_LENGTH, "entity %s", name);
            CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                          "Name too long !", free(arithmetic_source);
                                  free(sorted_by_appearance); fclose(arithmetic_file);)
            auto_error_t  err = find_pattern(pattern, source_off, 1, match);
            CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Regex exec failed !",
                                  free(arithmetic_source); free(sorted_by_appearance);
                                  fclose(arithmetic_file);)

            fwrite(source_off, sizeof(char), (size_t) match[0].rm_eo,
                   arithmetic_file);
            source_off += match[0].rm_eo;

            strncpy(pattern, "component (\\w+) is", MAX_NAME_LENGTH);

            err = find_pattern(pattern, source_off, 2, match);
            CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Regex exec failed !",
                                  free(arithmetic_source); free(sorted_by_appearance);
                                  fclose(arithmetic_file);)

            char component_name[MAX_NAME_LENGTH];
            memset(component_name, 0, sizeof(char) * MAX_NAME_LENGTH);
            size_t name_len = (size_t) (match[1].rm_eo - match[1].rm_so);
            strncpy(component_name, source_off + match[1].rm_so, name_len);
            component_name[name_len] = '\0';

            fwrite(source_off, sizeof(char), (size_t) match[1].rm_so,
                   arithmetic_file);
            source_off += match[1].rm_so;

            char hdl_file_name[MAX_NAME_LENGTH];
            memset(hdl_file_name, 0, sizeof(char) * MAX_NAME_LENGTH);
            char *last_slash = strrchr(op->hdl_path, '/');
            if (last_slash == NULL) {
                strncpy(hdl_file_name, op->hdl_path, MAX_NAME_LENGTH);
            } else {
                strncpy(hdl_file_name, last_slash + 1, MAX_NAME_LENGTH);
            }

            *strrchr(hdl_file_name, '.') = '\0';

            fwrite(hdl_file_name, sizeof(char), strlen(hdl_file_name),
                   arithmetic_file);
            source_off += (match[1].rm_eo - match[1].rm_so);
            fwrite(source_off, sizeof(char),
                   (size_t) (match[0].rm_eo - match[1].rm_eo), arithmetic_file);
            source_off = source_off + (size_t) (match[0].rm_eo - match[1].rm_eo);

            written = snprintf(
                    pattern, MAX_NAME_LENGTH,
                    "\\w+[[:space:]]*\\:[[:space:]]*component[[:space:]]*(%s)",
                    component_name);
            CHECK_COND_DO(written >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
                          "Name too long !", free(arithmetic_source);
                                  free(sorted_by_appearance); fclose(arithmetic_file);)


            err = find_pattern(pattern, source_off, 2, match);
            CHECK_COND_DO(err != 0, ERR_REGEX, "Regex exec failed !",
                                  free(arithmetic_source); free(sorted_by_appearance);
                                  fclose(arithmetic_file);)

            fwrite(source_off, sizeof(char), (size_t) match[1].rm_so,
                   arithmetic_file);
            source_off += match[0].rm_eo;

            fwrite(hdl_file_name, sizeof(char), strlen(hdl_file_name),
                   arithmetic_file);
        }
    }

    fwrite(source_off, sizeof(char), strlen(source_off), arithmetic_file);
    fclose(arithmetic_file);
    free(arithmetic_source);
    free(sorted_by_appearance);

    return ERR_NONE;
}

auto_error_t update_latency(float_op_t *op) {
    CHECK_PARAM(op)
    char *tcl_script = read_file(op->script_path, NULL);
    CHECK_NULL(tcl_script, ERR_IO, "Could not read tcl_script !")

    // TODO: Do it in a temp file
    FILE *script_file = fopen(op->script_path, "w");
    CHECK_COND_DO(script_file == NULL, ERR_IO, "Could not open script file !",
                  free(tcl_script);)

    char *offset = tcl_script;

    regmatch_t match[3];

    auto_error_t  err = find_pattern("c_latency([[:space:]]*)([[:digit:]]*)", tcl_script, 3, match);
    CHECK_COND_DO(err != ERR_NONE, ERR_REGEX, "Regex exec failed !",
            free(tcl_script); fclose(script_file);)

    offset += match[2].rm_so;
    fwrite(tcl_script, sizeof(char), (size_t) (offset - tcl_script),
           script_file);

    char latency_str[MAX_NAME_LENGTH];
    memset(latency_str, 0, sizeof(char) * MAX_NAME_LENGTH);
    snprintf(latency_str, MAX_NAME_LENGTH, "%" PRIu8, op->latency);

    fwrite(latency_str, sizeof(char), strlen(latency_str), script_file);
    offset = tcl_script + match[0].rm_eo;

    fwrite(offset, sizeof(char), strlen(offset), script_file);

    fclose(script_file);

    free(tcl_script);

    return ERR_NONE;
}

auto_error_t update_fop_tcl(vivado_hls_t *hls) {
    CHECK_PARAM(hls)
    CHECK_PARAM(hls->float_ops)

    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t *op = &(hls->float_ops[i]);
        update_latency(op);
    }

    return ERR_NONE;
}

void free_floats(vivado_hls_t *hls) {
    for (uint8_t i = 0; i < hls->nb_float_op; ++i) {
        float_op_t *op = &(hls->float_ops[i]);
        if (op != NULL) {
            free_str_arr(op->arith_unit_name_list,
                         (uint8_t) (op->nb_arith_names));
        }
    }
    free(hls->float_ops);
}

auto_error_t resolve_float_ops(vivado_hls_t *hls, hdl_info_t *hdl_info) {
    CHECK_CALL(find_float_op(hls), "find_float_op failed !")
    CHECK_CALL(open_dot_file(hls, hdl_info), "open_dot_file failed !")
    CHECK_CALL(update_fop_tcl(hls), "update_fop_tcl failed !")
    return ERR_NONE;
}

auto_error_t hls_free(vivado_hls_t *hls) {
    CHECK_PARAM(hls)
    CHECK_PARAM(hls->hls_source)
    if (hls->hls_source != NULL) {
        free(hls->hls_source);
        hls->hls_source = NULL;
    }
    if (hls->float_ops != NULL) {
        free_floats(hls);
        hls->float_ops = NULL;
    }
    return ERR_NONE;
}
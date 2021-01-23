#define _XOPEN_SOURCE 500
#include "utils.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

char *get_source(const char *path, size_t *file_size) {
    if (path == NULL) {
        fprintf(stderr, "Path is NULL !\n");
        return NULL;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file !\n");
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size_t size = (size_t)ftell(file);
    rewind(file);

    char *source = (char *)malloc(size + 1);
    if (source == NULL) {
        fprintf(stderr,
                "Memory allocation for the file data buffer has failed !\n");
        fclose(file);
        return NULL;
    }
    size_t read = fread(source, sizeof(char), size, file);
    if (read < size) {
        return NULL;
    }
    fclose(file);
    if (file_size != NULL) {
        *file_size = size;
    }
    source[size] = '\0';
    return source;
}

auto_error_t get_string(char *name, const char *msg, int length) {
    CHECK_PARAM(name);
    CHECK_PARAM(msg);

    printf("%s\n", msg);
    char *result = fgets(name, length, stdin);
    CHECK_COND(result == NULL, ERR_IO,
               "There was an error while reading the input !");
    name[strlen(name) - 1] = '\0';
    return ERR_NONE;
}

void str_toupper(char *str) {
    while (*str != '\0') {
        *str = (char)toupper((int)*str);
        str++;
    }
}

void str_tolower(char *str) {
    while (*str != '\0') {
        *str = (char)tolower((int)*str);
        str++;
    }
}

auto_error_t get_name(char *name, const char *msg) {
    CHECK_CALL(get_string(name, msg, MAX_NAME_LENGTH), "get_string failed !");
    return ERR_NONE;
}

auto_error_t get_path(char *path, const char *msg, bool must_exist) {
    char temp[MAX_PATH_LENGTH];
    memset(temp, 0, sizeof(char) * MAX_NAME_LENGTH);
    CHECK_CALL(get_string(temp, msg, MAX_PATH_LENGTH), "get_string failed !");
    char *final_path = realpath(temp, path);
    if (final_path == NULL) {
        if (must_exist) {
            return ERR_IO;
        }
        char **path_components;
        uint8_t allocated = 0;
        CHECK_CALL(allocate_str_arr(&path_components, &allocated),
                   "allocate_str_arr failed !");
        size_t nb_components = 0;
        bool fully_resolved_path = false;
        while (final_path == NULL && !fully_resolved_path) {
            char *last_comp = strrchr(temp, '/');
            if (last_comp == NULL) {
                fully_resolved_path = true;
            } else {
                if (nb_components == allocated) {
                    CHECK_CALL_DO(
                        allocate_str_arr(&path_components, &allocated),
                        "allocate_str_arr failed !",
                        free_str_arr(path_components, allocated));
                }
                if(strcmp(last_comp + 1, strrchr(path, '/') + 1) == 0) {
                    fully_resolved_path = true;
                }
                else {
                    strncpy(path_components[nb_components], last_comp,
                        strlen(last_comp));
                    nb_components++;
                    *last_comp = '\0';
                }
            }
        }
        if(nb_components >= 1) {
            for (size_t i = nb_components - 1; i > 0; --i) {
                strncat(path, path_components[i], MAX_NAME_LENGTH);
            }
            strncat(path, path_components[0], MAX_NAME_LENGTH);
        }
        
        free_str_arr(path_components, allocated);
    }

    CHECK_COND(final_path == NULL && errno != ENOENT, ERR_PATH,
               "Could not resolve absolute path !");
    return ERR_NONE;
}

void free_str_arr(char **str_arr, uint8_t last) {
    if (str_arr == NULL) {
        return;
    }
    for (uint8_t i = 0; i < last; ++i) {
        if (str_arr[i] != NULL) {
            free(str_arr[i]);
        }
    }
    free(str_arr);
}

auto_error_t allocate_str_arr(char ***str_arr, uint8_t *last) {
    CHECK_PARAM(str_arr);
    CHECK_PARAM(last);
    char **new_str_arr;
    uint8_t prev_last = *last;
    if (*last == 0) {
        new_str_arr = calloc(5, sizeof(char *));
        *last = 5;
    } else {
        new_str_arr =
            realloc(*str_arr, (size_t)(*last * (size_t)2 * sizeof(char *)));
        *last = (uint8_t)(*last * 2);
    }
    CHECK_COND_DO(new_str_arr == NULL, ERR_MEM,
                  "Could not realloc for str_arr !",
                  free_str_arr(*str_arr, prev_last););
    *str_arr = new_str_arr;

    for (uint8_t i = prev_last; i < *last; ++i) {
        (*str_arr)[i] = calloc(MAX_PATH_LENGTH, sizeof(char));
        CHECK_COND_DO((*str_arr)[i] == NULL, ERR_MEM,
                      "Could not realloc for str_arr !",
                      free_str_arr(*str_arr, i););
    }
    return ERR_NONE;
}

//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wunused-parameter"
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag,
              struct FTW *ftwbuf) {
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}
//#pragma clang diagnostic pop

void clean_folder() {
    nftw("vivado_hls", unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    nftw(".Xil", unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    remove("final_script.tcl");
    remove("generate_axi_ip.tcl");
    remove("generate_project.tcl");
    remove("hls_script.tcl");
    remove("vivado.jou");
    remove("vivado.log");
    remove("vivado_hls.log");

    DIR *d;
    d = opendir("./");
    if (d == NULL) {
        return;
    }

    regex_t reg;
    regmatch_t match[1];

    int err = regcomp(&reg,
                      "(vivado_[[:digit:]]*\\.backup)|address_adapter|mem_"
                      "interface|write_enb_adapter",
                      REG_EXTENDED);
    if (err != 0) {
        closedir(d);
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        err = regexec(&reg, dir->d_name, 1, match, 0);
        if (err == 0) {
            remove(dir->d_name);
        }
    }

    regfree(&reg);

    closedir(d);
}
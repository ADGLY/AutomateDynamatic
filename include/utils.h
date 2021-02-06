#pragma once

#include "error.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_NAME_LENGTH NAME_MAX
#define MAX_PATH_LENGTH PATH_MAX

#define GET_SUFFIX(arr, str)                                                   \
    const char *str;                                                           \
    if ((arr)->write && (arr)->read) {                                         \
        str = "read_write";                                                    \
    } else if ((arr)->write) {                                                 \
        str = "write";                                                         \
    } else if ((arr)->read) {                                                  \
        str = "read";                                                          \
    }

/**
 * Takes in a path to a file and read the content of said file
 *
 * @param path the path to the file to be read
 * @param file_size If not NULL, will be filled with the file size
 *
 * @return the contents of the file
 **/
char* read_file(const char* path, size_t* file_size);

/**
 * Transforms the given string into an upper case string
 *
 * @param str the string to be transformed
 *
 **/
void str_toupper(char* str);

/**
 * Transforms the given string into an upper case string
 *
 * @param str the string to be transformed
 *
 **/
void str_tolower(char* str);

/**
 * Prints a message and asks for text input
 *
 * @param name the string to fill with the input
 * @param msg the mesg to be print
 *
 * @return an error codes
 */
auto_error_t get_name(char* name, const char* msg);

/**
 * Prints a message and asks for a path. The path might exist or not.
 * If it does not exist and must_exist is false, the path will be resolved.
 *
 * @param path the string to fill with the path
 * @param msg the message to print
 * @param must_exist indicates if the file must exist or not
 *
 * @return an error code
 *
 */
auto_error_t get_path(char* path, const char* msg, bool must_exist);

/**
 * Frees the given array of strings
 *
 * @param str_arr the string array to free
 * @param nb_allocated the number of allocated elements
 *
 **/
void free_str_arr(char** str_arr, uint8_t nb_allocated);

/**
 * Allocates or reallocates a string array
 *
 * @param str_arr pointer to the soon to be allocated array
 * @param nb_allocated the number of allocated strings the array
 *
 * @return an error code
 *
 **/
auto_error_t allocate_str_arr(char*** str_arr, uint8_t* nb_allocated);

/**
 * Removes anything temporary created by the program
 *
 **/
void clean_folder();
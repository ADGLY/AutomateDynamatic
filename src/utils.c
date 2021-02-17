#define _XOPEN_SOURCE 500
#include "utils.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "regex_wrapper.h"


char* read_file(const char* path, size_t* file_size) {
	if (path == NULL) {
		fprintf(stderr, "Path is NULL !\n");
		return NULL;
	}
	FILE* file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file !\n");
		return NULL;
	}
	fseek(file, 0L, SEEK_END);
	size_t size = (size_t)ftell(file);
	rewind(file);

	char* source = (char*)malloc(size + 1);
	if (source == NULL) {
		fprintf(stderr,
			"Memory allocation for the file data buffer has failed !\n");
		fclose(file);
		return NULL;
	}
	size_t read = fread(source, sizeof(char), size, file);
	if (read < size) {
		free(source);
		fclose(file);
		return NULL;
	}
	fclose(file);
	if (file_size != NULL) {
		*file_size = size;
	}
	source[size] = '\0';
	return source;
}

char* remove_leading_whitespace(char* str) {
	if (strlen(str) == 0) {
		return str;
	}
	while (*str != '\0' && isspace(*str)) {
		str++;
	}
	return str;
}

void remove_trailing_whitespace(char* str) {
	if (strlen(str) == 0) {
		return;
	}
	char* str_temp = str + strlen(str) - 1;
	while (strlen(str) > 0 && isspace(*str_temp)) {
		*str_temp = '\0';
		str_temp--;
	}
}

/**
* Get a string from stdin
*
* @param str the string to fill with the input
* @param msg the message to print
* @param length the number of chars str can store
*
* @return an error code
*
**/
auto_error_t get_str(char* str, const char* msg, int length) {
	CHECK_PARAM(str);
	CHECK_PARAM(msg);

	printf("%s\n", msg);

	char* str_temp = malloc((size_t)length);
	if (str_temp == NULL) {
		return ERR_MEM;
	}

	char* result = fgets(str_temp, length, stdin);
	CHECK_COND(result == NULL, ERR_IO,
		"There was an error while reading the input !");
	remove_trailing_whitespace(str_temp);
	char* no_lead_space = remove_leading_whitespace(str_temp);
	strncpy(str, no_lead_space, (size_t)length);
	free(str_temp);

	return ERR_NONE;
}

void str_toupper(char* str) {
	while (*str != '\0') {
		*str = (char)toupper((int)*str);
		str++;
	}
}

void str_tolower(char* str) {
	while (*str != '\0') {
		*str = (char)tolower((int)*str);
		str++;
	}
}

auto_error_t get_name(char* name, const char* msg) {
	CHECK_CALL(get_str(name, msg, MAX_NAME_LENGTH), "get_string failed !");
	return ERR_NONE;
}

auto_error_t resolve_path(bool must_exist, char* path, char* temp_path) {
	char* final_path = realpath(temp_path, path);
	if (final_path == NULL) {
		if (must_exist) {
			return ERR_IO;
		}
		char** path_components;
		uint8_t allocated = 0;
		CHECK_CALL(allocate_str_arr(&path_components, &allocated),
			"allocate_str_arr failed !");
		size_t nb_components = 0;
		bool fully_resolved_path = false;
		while (final_path == NULL && !fully_resolved_path) {
			char* last_comp = strrchr(temp_path, '/');
			if (last_comp == NULL) {
				fully_resolved_path = true;
			}
			else {
				if (nb_components == allocated) {
					CHECK_CALL_DO(
						allocate_str_arr(&path_components, &allocated),
						"allocate_str_arr failed !",
						free_str_arr(path_components, allocated));
				}
				if (strcmp(last_comp + 1, strrchr(path, '/') + 1) == 0) {
					fully_resolved_path = true;
				}
				else {
					strcpy(path_components[nb_components], last_comp);
					nb_components++;
					*last_comp = '\0';
				}
			}
		}
		if (nb_components >= 1) {
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

auto_error_t get_path(char* path, const char* msg, bool must_exist) {
	char temp_resolved_path[MAX_PATH_LENGTH];
	memset(temp_resolved_path, 0, sizeof(char) * MAX_NAME_LENGTH);

	CHECK_CALL(get_str(temp_resolved_path, msg, MAX_PATH_LENGTH), "get_string failed !");

	CHECK_CALL(resolve_path(must_exist, path, temp_resolved_path), "resolve_path failed !");
	return ERR_NONE;
}

auto_error_t allocate_str_arr(char*** str_arr, uint8_t* nb_allocated) {
	CHECK_PARAM(str_arr);
	CHECK_PARAM(nb_allocated);
	char** new_str_arr;
	uint8_t prev_last = *nb_allocated;
	if (*nb_allocated == 0) {
		new_str_arr = calloc(5, sizeof(char*));
		*nb_allocated = 5;
	}
	else {
		new_str_arr =
			realloc(*str_arr, (size_t)(*nb_allocated * (size_t)2 * sizeof(char*)));
		*nb_allocated = (uint8_t)(*nb_allocated * 2);
	}
	CHECK_COND_DO(new_str_arr == NULL, ERR_MEM,
		"Could not realloc for str_arr !",
		free_str_arr(*str_arr, prev_last););
	*str_arr = new_str_arr;

	for (uint8_t i = prev_last; i < *nb_allocated; ++i) {
		(*str_arr)[i] = calloc(MAX_PATH_LENGTH, sizeof(char));
		CHECK_COND_DO((*str_arr)[i] == NULL, ERR_MEM,
			"Could not realloc for str_arr !",
			free_str_arr(*str_arr, i););
	}
	return ERR_NONE;
}

void free_str_arr(char** str_arr, uint8_t nb_allocated) {
	if (str_arr == NULL) {
		return;
	}
	for (uint8_t i = 0; i < nb_allocated; ++i) {
		if (str_arr[i] != NULL) {
			free(str_arr[i]);
		}
	}
	free(str_arr);
}

int unlink_cb(const char* fpath, const struct stat* sb, int typeflag,
	struct FTW* ftwbuf) {
	int rv = remove(fpath);

	if (rv)
		perror(fpath);

	return rv;
}

auto_error_t grow_array(void** array, size_t* prev_alloc, size_t elem_size) {

	*prev_alloc *= 2;
	void* new_array =
		realloc(array, *prev_alloc * elem_size);
	CHECK_COND(new_array == NULL, ERR_MEM, "Failed to realloc !");
	*array = new_array;

	return ERR_NONE;

}

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

	DIR* d;
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

	struct dirent* dir;
	while ((dir = readdir(d)) != NULL) {
		err = find_pattern_compiled(&reg, dir->d_name, 1, match);
		if (err == ERR_NONE) {
			remove(dir->d_name);
		}
	}

	regfree(&reg);

	closedir(d);
}
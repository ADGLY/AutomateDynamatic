#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdl.h"
#include "regex_wrapper.h"


static const char* const write_ports[NB_BRAM_INTERFACE] = {
	"_address0", "_ce0", "_we0", "_dout0", "_din0" };

static const char* const read_ports[NB_BRAM_INTERFACE] = {
	"_address1", "_ce1", "_we1", "_dout1", "_din1" };


auto_error_t hdl_create(hdl_info_t* hdl_info) {
	CHECK_PARAM(hdl_info);

	memset(hdl_info, 0, sizeof(hdl_info_t));

	auto_error_t err =
		get_path(hdl_info->dir,
			"What is the directory of the Dynamatic output (hdl) ?", true);
	while (err == ERR_IO) {
		fprintf(stderr,
			"The path does not exist. Please enter a valid path !\n");
		err = get_path(hdl_info->dir,
			"What is the directory of the Dynamatic output (hdl) ?",
			true);
	}

	CHECK_CALL(get_name(hdl_info->top_file_name,
		"What is the name of the top file ?"),
		"get_name failed !");

	strncpy(hdl_info->top_file_path, hdl_info->dir, MAX_PATH_LENGTH);

	//2 for the added '/' and the null byte
	CHECK_COND(strlen(hdl_info->dir) + strlen(hdl_info->top_file_name) + 2 > MAX_PATH_LENGTH, ERR_NAME_TOO_LONG, "");
	hdl_info->top_file_path[strlen(hdl_info->dir)] = '/';
	strcat(hdl_info->top_file_path, hdl_info->top_file_name);

	return ERR_NONE;
}

auto_error_t get_end_of_ports_decl(hdl_info_t* hdl_info) {
	regmatch_t match[1];
	CHECK_CALL(find_pattern("end;", hdl_info->source, 1, match), "find_pattern failed !");

	hdl_info->end_of_ports_decl = (size_t)match[0].rm_eo;

	return ERR_NONE;
}

auto_error_t get_entity_name(hdl_info_t* hdl_info) {
	regmatch_t match[2];
	CHECK_CALL(find_pattern("entity[[:space:]]*(\\w+)[[:space:]]*is", hdl_info->source, 2, match), "find_pattern failed !");

	CHECK_LENGTH((size_t)(match[1].rm_eo - match[1].rm_so), MAX_NAME_LENGTH);
	strncpy(hdl_info->name, hdl_info->source + match[1].rm_so,
		(size_t)(match[1].rm_eo - match[1].rm_so));

	return ERR_NONE;
}

uint16_t get_width(regmatch_t* matches, const char* str, uint8_t idx) {
	char str_width[MAX_NAME_LENGTH];
	memset(str_width, 0, MAX_NAME_LENGTH * sizeof(char));

	size_t len = (size_t)(matches[idx].rm_eo - matches[idx].rm_so);
	CHECK_COND(len >= MAX_NAME_LENGTH, ERR_NAME_TOO_LONG,
		"Name too long !");

	strncpy(str_width, str, len);

	uint16_t width = (uint16_t)(strtoll(str_width, NULL, 10) + 1);
	return width;
}

auto_error_t get_arrays(hdl_info_t* hdl_info) {
	CHECK_PARAM(hdl_info);
	CHECK_PARAM(hdl_info->source);

	const char* source_off = hdl_info->source;

	size_t alloc_size = 10;
	hdl_array_t* arrays = calloc(alloc_size, sizeof(hdl_array_t));
	CHECK_NULL(arrays, ERR_MEM, "Failed to allocate memory for arrays !");
	size_t array_count = 0;

	size_t end_of_port = hdl_info->end_of_ports_decl;

	regmatch_t matches[3];
	set_pattern("(\\w+)_din1[[:space:]]*\\:[[:space:]]*in[[:space:]]*std_"
		"logic_vector[[:space:]]*\\(([[:digit:]]+)");


	auto_error_t err;
	while ((size_t)(source_off - hdl_info->source) <= end_of_port &&
		(err = find_set_pattern(source_off, 3, matches)) != ERR_REGEX) {

		CHECK_COND_DO((size_t)(matches[1].rm_eo - matches[1].rm_so) >=
			MAX_NAME_LENGTH,
			ERR_NAME_TOO_LONG, "",
			free((void*)arrays););
		strncpy(arrays[array_count].name, source_off + matches[0].rm_so,
			(size_t)(matches[1].rm_eo - matches[1].rm_so));

		uint16_t width = get_width(matches, source_off + matches[2].rm_so, 2);
		arrays[array_count].width = width;

		array_count++;
		source_off += matches[0].rm_eo;

		//Realloc array
		if (array_count == alloc_size) {
			CHECK_CALL_DO(grow_array((void**)(&arrays), &alloc_size, sizeof(hdl_array_t)), "grow_array failed !", free(arrays););
		}

	}

	if (array_count != 0) {
		hdl_array_t* new_arrays =
			realloc(arrays, array_count * sizeof(hdl_array_t));
		CHECK_COND_DO(new_arrays == NULL, ERR_MEM, "Failed to realloc !",

			free(arrays));
		hdl_info->arrays = new_arrays;
		hdl_info->nb_arrays = array_count;
		hdl_info->end_arrays = (size_t)(source_off - hdl_info->source);
	}

	return ERR_NONE;
}

auto_error_t fill_interfaces(bram_interface_t* read_interface,
	bram_interface_t* write_interface,
	const char* arr_name) {
	CHECK_PARAM(read_interface);
	CHECK_PARAM(write_interface);
	CHECK_PARAM(arr_name);

	char* pointer_to_read = (char*)read_interface;
	char* pointer_to_write = (char*)write_interface;

	for (int i = 0; i < NB_BRAM_INTERFACE; ++i) {
		size_t name_len = strlen(arr_name);

		size_t read_total_len = name_len + strlen(read_ports[i]) + 1;
		CHECK_LENGTH(read_total_len, MAX_NAME_LENGTH);

		strncpy(pointer_to_read, arr_name, MAX_NAME_LENGTH);
		CHECK_COND(MAX_NAME_LENGTH - name_len > MAX_NAME_LENGTH ||
			MAX_NAME_LENGTH - name_len < strlen(read_ports[i]) + 1,
			ERR_NAME_TOO_LONG, "Name too long !");
		strncpy(pointer_to_read + name_len, read_ports[i],
			MAX_NAME_LENGTH - name_len);

		size_t write_total_len = name_len + strlen(write_ports[i]) + 1;
		CHECK_LENGTH(write_total_len, MAX_NAME_LENGTH);

		strncpy(pointer_to_write, arr_name, MAX_NAME_LENGTH);
		CHECK_COND(MAX_NAME_LENGTH - name_len > MAX_NAME_LENGTH ||
			MAX_NAME_LENGTH - name_len < strlen(write_ports[i]) + 1,
			ERR_NAME_TOO_LONG, "Name too long !");
		strncpy(pointer_to_write + name_len, write_ports[i],
			MAX_NAME_LENGTH - name_len);

		pointer_to_read += MAX_NAME_LENGTH;
		pointer_to_write += MAX_NAME_LENGTH;
	}
	return ERR_NONE;
}

auto_error_t fill_arrays_ports(hdl_info_t* hdl_info) {
	CHECK_PARAM(hdl_info);
	CHECK_PARAM(hdl_info->arrays);

	for (size_t i = 0; i < hdl_info->nb_arrays; ++i) {
		hdl_array_t* array = &(hdl_info->arrays[i]);
		const char* arr_name = array->name;
		CHECK_CALL(fill_interfaces(&(array->read_ports), &(array->write_ports),
			arr_name),
			"fill_interfaces failed !");
	}
	return ERR_NONE;
}


auto_error_t get_params(hdl_info_t* hdl_info) {
	CHECK_PARAM(hdl_info);
	CHECK_PARAM(hdl_info->source);

	const char* source_off = hdl_info->source;

	size_t alloc_size = 10;
	hdl_in_param_t* params = calloc(alloc_size, sizeof(hdl_array_t));
	CHECK_NULL(params, ERR_MEM, "Failed to allocate memory for arrays !");
	size_t param_count = 0;

	size_t end_of_port = hdl_info->end_of_ports_decl;

	regmatch_t matches[3];
	set_pattern("(\\w+)_din[[:space:]]*\\:[[:space:]]*in[[:space:]]*std_"
		"logic_vector[[:space:]]*\\(([[:digit:]]+)");


	auto_error_t err;
	while ((size_t)(source_off - hdl_info->source) <= end_of_port &&
		(err = find_set_pattern(source_off, 3, matches)) != ERR_REGEX) {

		CHECK_COND_DO((size_t)(matches[1].rm_eo - matches[1].rm_so) >=
			MAX_NAME_LENGTH,
			ERR_NAME_TOO_LONG, "",
			free((void*)params););
		strncpy(params[param_count].name, source_off + matches[0].rm_so,
			(size_t)(matches[1].rm_eo - matches[1].rm_so));

		uint16_t width = get_width(matches, source_off + matches[2].rm_so, 2);
		params[param_count].width = width;

		param_count++;
		source_off += matches[0].rm_eo;

		//Realloc array
		if (param_count == alloc_size) {
			CHECK_CALL_DO(grow_array((void**)(&params), &alloc_size, sizeof(hdl_in_param_t)), "grow_array failed !", free(params););
		}

	}

	if (param_count != 0) {
		hdl_in_param_t* new_params =
			realloc(params, param_count * sizeof(hdl_in_param_t));
		CHECK_COND_DO(new_params == NULL, ERR_MEM, "Failed to realloc !",

			free(params));
		hdl_info->params = new_params;
		hdl_info->nb_params = param_count;
	}

	return ERR_NONE;
}

auto_error_t get_end_out_width(hdl_info_t* hdl_info) {
	regmatch_t match[2];
	CHECK_CALL(find_pattern("end_out[[:space:]]*:[[:space:]]*out[[:space:]]*std_logic_vector[[:space:]]*\\(([[:digit:]]+)", hdl_info->source, 2, match), "find_pattern failed !");
	
	uint16_t width = get_width(match, hdl_info->source + match[1].rm_so, 1);
	hdl_info->end_out_width = width;
	return ERR_NONE;
}

auto_error_t parse_hdl(hdl_info_t* hdl_info) {
	CHECK_PARAM(hdl_info);
	char* source = read_file(hdl_info->top_file_path, NULL);
	CHECK_NULL(source, ERR_IO, "Did not manage to read source file !");
	hdl_info->source = source;

	CHECK_CALL(get_end_of_ports_decl(hdl_info),
		"get_end_of_ports_decl failed !");
	CHECK_CALL(get_end_out_width(hdl_info), "get_end_out_width failed !");
	CHECK_CALL(get_entity_name(hdl_info), "get_entity_name failed !");
	CHECK_CALL(get_arrays(hdl_info), "get_arrays failed !");
	CHECK_CALL(fill_arrays_ports(hdl_info), "fill_arrays_ports failed !");
	CHECK_CALL(get_params(hdl_info), "get_params failed !");

	uint16_t max_width = hdl_info->end_out_width;
	for (size_t i = 0; i < hdl_info->nb_params; ++i) {
		if (hdl_info->params[i].width > max_width) {
			max_width = hdl_info->params[i].width;
		}
	}

	if (max_width < 32) {
		max_width = 32;
	}

	hdl_info->max_param_width = max_width;

	return ERR_NONE;
}

auto_error_t hdl_free(hdl_info_t* hdl_info) {
	CHECK_PARAM(hdl_info);
	if (hdl_info->arrays != NULL) {
		free(hdl_info->arrays);
		hdl_info->arrays = NULL;
	}
	if (hdl_info->params != NULL) {
		free(hdl_info->params);
		hdl_info->params = NULL;
	}
	if (hdl_info->source != NULL) {
		free(hdl_info->source);
		hdl_info->source = NULL;
	}
	return ERR_NONE;
}
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "vivado.h"

bool is_power_of_two(size_t size)
{
    return (size & (size - 1)) == 0;
}

auto_error_t get_array_size(project_t *project)
{
    CHECK_PARAM(project);

    printf("How many 32bits words does an array store ?\n");
    printf("The maximum is 1G and the input should be of the form 4K, 8K, ..., 1M, ..., 1G\n");
    uint16_t array_size;
    int scanned = scanf("%" SCNu16, &array_size);
    CHECK_COND(scanned < 0 || scanned > 1 || array_size == 0 || array_size > 512 || !is_power_of_two(array_size), ERR_IO, "Wrong array size !");

    char ind = (char)getchar();
    CHECK_COND((ind != 'K' && ind != 'M' && ind != 'G') || (ind == 'G' && array_size > 1), ERR_IO, "Wrong size indicator !");

    char c = (char)getchar();
    CHECK_COND(c != '\n', ERR_IO, "You need to only input the size and the size indicator !");

    project->array_size = array_size;
    project->array_size_ind = ind;
    return ERR_NONE;
}

auto_error_t create_project(project_t *project, hdl_source_t *hdl_source)
{
    CHECK_PARAM(project);
    CHECK_PARAM(hdl_source);

    memset(project, 0, sizeof(project_t));
    project->hdl_source = hdl_source;

    CHECK_CALL(get_path(project->path, "What is the path of the Vivado project ?", false), "get_path failed !");
    CHECK_CALL(get_name(project->name, "What is the name of the Vivado project ?"), "get_name failed !");

    CHECK_CALL(get_array_size(project), "get_array_size failed !");

    return ERR_NONE;
}

auto_error_t launch_script(const char *name, const char *exec_path)
{
    CHECK_PARAM(name);
    CHECK_PARAM(exec_path);

    char script_path[MAX_PATH_LENGTH];
    memset(script_path, 0, sizeof(char) * MAX_PATH_LENGTH);
    strncpy(script_path, exec_path, MAX_PATH_LENGTH - 1);

    FILE *vivado_input;
    vivado_input = popen("vivado -mode tcl", "w");
    CHECK_COND(vivado_input == NULL, ERR_IO, "Failed to launch Vivado !");

    fprintf(vivado_input, "source %s/%s\n", script_path, name);
    fprintf(vivado_input, "exit\n");
    pclose(vivado_input);

    return ERR_NONE;
}

auto_error_t project_free(project_t *project)
{
    CHECK_PARAM(project);

    CHECK_CALL(hdl_free(project->hdl_source), "hdl_free failed !");
    return ERR_NONE;
}
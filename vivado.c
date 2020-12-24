#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include "vivado.h"

void get_project_path(project_t* project) {
    printf("What is the path of the Vivado project ?\n");
    char temp[MAX_NAME_LENGTH];
    char* result = fgets(temp, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    if(temp[0] != '/') {
        char* result = getcwd(project->path, MAX_NAME_LENGTH);
        if(result == NULL) {
            fprintf(stderr, "getcwd error !\n");
            return;
        }
        if(temp[0] != '.') {
            if(strlen(project->path) + 1 + strlen(temp) + 1 >= MAX_NAME_LENGTH) {
                fprintf(stderr, "Path is too long !\n");
            }
            project->path[strlen(project->path)] = '/';
            strcpy(project->path + strlen(project->path), temp);
        }
    }
    project->path[strlen(project->path) - 1] = '\0';

    return;
}

void get_project_name(project_t* project) {
    printf("What is the name of the Vivado project ?\n");
    char* result = fgets(project->name, MAX_NAME_LENGTH, stdin);
    if(result == NULL) {
        fprintf(stderr, "There was an error while reading the input !\n");
    }
    project->name[strlen(project->name) - 1] = '\0';

    return;
}

void create_project(project_t* project, hdl_source_t* hdl_source) {
    memset(project, 0, sizeof(project_t));
    get_project_path(project);
    get_project_name(project);
    strcpy(project->axi_ip.path, project->path);
    if(strlen(project->axi_ip.path) + strlen("/ip_repo") + 1 >= MAX_NAME_LENGTH) {
        fprintf(stderr, "The path for the IP is too long !\n");
    }
    strcpy(project->axi_ip.path + strlen(project->axi_ip.path), "/ip_repo");
    project->hdl_source = hdl_source;

    project->axi_ip.revision = 1;
}


void launch_script(const char* name) {
    FILE* vivado_input;
    vivado_input = popen("vivado -mode tcl", "w");
    if(vivado_input == NULL) {
        fprintf(stderr, "Failed to launch Vivado !\n");
    }

    char script_path[MAX_NAME_LENGTH];
    char* result = getcwd(script_path, MAX_NAME_LENGTH);
    if(result == NULL) {
        fprintf(stderr, "getcwd error !\n");
        return;
    }
    fprintf(vivado_input, "source %s/%s\n", script_path, name);
    fprintf(vivado_input, "exit\n");
    pclose(vivado_input);
}
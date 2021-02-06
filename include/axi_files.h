#pragma once

#include "vivado.h"

/**
 * Will contain the path and the content of the AXI IP files
 **/
typedef struct _axi_files {
    const char *top_file;
    char top_file_path[MAX_PATH_LENGTH];
    const char *axi_file;
    char axi_file_path[MAX_PATH_LENGTH];
} axi_files_t;

/**
 * Contains every relevant information for the AXI IP
 **/
typedef struct _axi_ip {
    char name[MAX_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
    char interface_name[MAX_NAME_LENGTH];
    size_t nb_slave_registers;
    axi_files_t axi_files;
    int revision;
    uint16_t data_width;
    uint16_t addr_width;
} axi_ip_t;

/**
 * Creates an axi_ip_t
 *
 * @param axi_ip the axi_ip_t struct that will be initialized
 * @param project project_t used for the path
 *
 * @return an error code
 **/

auto_error_t create_axi(axi_ip_t *axi_ip, project_t *project);

/**
 * Read the two AXI files, the top file and the AXI file
 *
 * @param axi_ip the axi_files struct it contains will be filled by this
 *function
 *
 * @return an error code
 **/
auto_error_t read_axi_files(axi_ip_t *axi_ip);

/**
 * Update the two AXI files. In the top file : instantiates the accelerator
 * expose the memory interface. In the AXI file, do the register mapping
 *
 * @param project Used to retrieve the hdl_source
 * @param axi_ip Used to get the path and content of the AXI IP files
 *
 * @return an error code
 **/
auto_error_t update_files(project_t *project, axi_ip_t *axi_ip);

/**
 * Free the axi_ip
 *
 * @param axi_ip the axi_ip to be freed
 *
 * @return an error code
 **/
auto_error_t free_axi(axi_ip_t *axi_ip);
#include "file.h"
#include "parser.h"
#include "tcl.h"



//histogram_elaborated_optimized.vhd

int main(void) {

    char hdl_path[MAX_NAME_LENGTH];
    get_hdl_path(hdl_path);

    hdl_source_t* hdl_source = parse_hdl(hdl_path);
    create_AXI_script(hdl_source);

    return 0;
}
#include "file.h"
#include "hdl_data.h"
#include "tcl.h"



//histogram_elaborated_optimized.vhd

int main(void) {

    hdl_source_t* hdl_source = hdl_create();
    parse_hdl(hdl_source);
    create_AXI_script(hdl_source);

    return 0;
}
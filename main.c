#include "file.h"
#include "parser.h"



//histogram_elaborated_optimized.vhd

int main(void) {

    char hdl_path[MAX_NAME_LENGTH];
    get_hdl_path(hdl_path);

    hdl_source_t* source = parse_hdl(hdl_path);

    return 0;
}
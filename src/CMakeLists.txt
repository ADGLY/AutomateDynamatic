set(HEADER_FILES "${AutomateDynamatic_SOURCE_DIR}/include/axi_files.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/error.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/hdl.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/tcl_board.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/tcl.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/utils.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/vivado_hls.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/vivado.h"
                 "${AutomateDynamatic_SOURCE_DIR}/include/regex_wrapper.h")
                 
set(SOURCE_LIST "${AutomateDynamatic_SOURCE_DIR}/src/axi_files.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/hdl.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/tcl_board.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/tcl.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/utils.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/vivado_hls.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/vivado.c"
                "${AutomateDynamatic_SOURCE_DIR}/src/regex_wrapper.c")

add_library(automate_dynamatic_library ${SOURCE_LIST} ${HEADER_FILES})

target_include_directories(automate_dynamatic_library PUBLIC ../include)

target_compile_features(automate_dynamatic_library PUBLIC c_std_11)

target_compile_options(automate_dynamatic_library PRIVATE ${WARNINGS_LIST})

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_FILES})

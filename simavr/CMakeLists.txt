# -*- CMake -*-

#################################################################################

file(GLOB ALL_SIMAVR_CORES_SRC cores/*.c)
foreach(FILENAME ${ALL_SIMAVR_CORES_SRC})
  # try_compile(RESULT "out" ${FILENAME}
  #   CMAKE_C_FLAGS "-E --std=gnu99 -Wall -I${AVR_INCLUDE_DIRECTORY} -I${CMAKE_CURRENT_DIR}/include"
  #   )
  execute_process(
    COMMAND
    ${CMAKE_C_COMPILER} -E --std=gnu99 -I${AVR_INCLUDE_DIRECTORY} -I${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FILENAME}
    RESULT_VARIABLE RC
    OUTPUT_QUIET
    ERROR_QUIET
    )
  if(RC)
    message(WARNING "${FILENAME} did not compile, check your avr-gcc toolchain")
  else(RC)
    get_filename_component(CORE ${FILENAME} NAME_WE)
    string(REPLACE "sim_" "" CORE ${CORE})
    string(TOUPPER ${CORE} CORE)
    list(APPEND DEFINES_LIST "#define CONFIG_${CORE} 1\n")
    # avr_kind_t usb162 = {
    file(STRINGS ${FILENAME} CORE_LOWER REGEX "avr_kind_t")
    list(APPEND SIMAVR_CORES_SRC ${FILENAME})
    if(CORE_LOWER)
      string(REPLACE "avr_kind_t " "" CORE_LOWER ${CORE_LOWER})
      string(REPLACE " = {" "" CORE_LOWER ${CORE_LOWER})
      # message("${FILENAME} ${CORE} ${CORE_LOWER}")
      list(APPEND AVR_KIND_T_LIST "#if CONFIG_${CORE}\nextern avr_kind_t ${CORE_LOWER}\;\n#endif\n")
      list(APPEND AVR_KIND_LIST "#if CONFIG_${CORE}\n\t&${CORE_LOWER},\n#endif\n")
    endif(CORE_LOWER)
  endif(RC)
endforeach(FILENAME)
string(CONCAT DEFINES ${DEFINES_LIST})
string(CONCAT AVR_KIND_T ${AVR_KIND_T_LIST})
string(CONCAT AVR_KIND ${AVR_KIND_LIST})
configure_file(sim_core_config.h.in sim_core_config.h @ONLY)
configure_file(sim_core_decl.h.in sim_core_decl.h @ONLY)

#################################################################################

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
  )

# set_property(SOURCE ${SIMAVR_SIM_SRC} PROPERTY INCLUDE_DIRECTORIES
#   ...
#   )

file(GLOB SIMAVR_SIM_SRC source/avr_*.c source/sim_*.c)
# set_source_files_properties(${SIMAVR_SIM_SRC} PROPERTIES
#   COMPILE_FLAGS "-I${CMAKE_CURRENT_LIST_DIR} -I${CMAKE_CURRENT_LIST_DIR}/sim"
#   )

# file(GLOB SIMAVR_CORES_SRC cores/*.c)
set(C_FLAGS_CORES "-nostdinc -DAVR_CORE=1")
set_source_files_properties(${SIMAVR_CORES_SRC} PROPERTIES
 COMPILE_FLAGS "${C_FLAGS_CORES} -I${AVR_INCLUDE_DIRECTORY}")

add_library(simavr SHARED
  sim_core_config.h sim_core_decl.h
  ${SIMAVR_SIM_SRC} ${SIMAVR_CORES_SRC}
  )
target_link_libraries(simavr elf)
install(TARGETS simavr LIBRARY DESTINATION lib)

install(DIRECTORY include/*.h # sim_core_*.h  
  DESTINATION include/simavr
  FILES_MATCHING PATTERN "*.h"
  )

install(DIRECTORY include/avr/*.h
  DESTINATION include/simavr/avr
  FILES_MATCHING PATTERN "*.h"
  )

add_executable(run_avr source/run_avr.c)
target_link_libraries(run_avr simavr)
install(TARGETS run_avr RUNTIME DESTINATION bin)

####################################################################################################
#
# End
#
####################################################################################################

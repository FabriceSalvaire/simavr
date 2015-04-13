# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/sim
  )

# set_property(SOURCE ${SIMAVR_SIM_SRC} PROPERTY INCLUDE_DIRECTORIES
#   ...
#   )

file(GLOB SIMAVR_SIM_SRC sim/avr_*.c sim/sim_*.c)
# set_source_files_properties(${SIMAVR_SIM_SRC} PROPERTIES
#   COMPILE_FLAGS "-I${CMAKE_CURRENT_LIST_DIR} -I${CMAKE_CURRENT_LIST_DIR}/sim"
#   )

file(GLOB SIMAVR_CORES_SRC cores/*.c)
set(C_FLAGS_CORES "-nostdinc -DAVR_CORE=1")
set_source_files_properties(${SIMAVR_CORES_SRC} PROPERTIES
 COMPILE_FLAGS "${C_FLAGS_CORES} -I${AVR_INCLUDE_DIRECTORY}")

add_library(simavr SHARED ${SIMAVR_SIM_SRC} ${SIMAVR_CORES_SRC})
target_link_libraries(simavr elf)
install(TARGETS simavr LIBRARY DESTINATION lib)

install(DIRECTORY sim/*.h # sim_core_*.h  
  DESTINATION include/simavr
  FILES_MATCHING PATTERN "*.h"
  )

install(DIRECTORY sim/avr/*.h
  DESTINATION include/simavr/avr
  FILES_MATCHING PATTERN "*.h"
  )

add_executable(run_avr sim/run_avr.c)
target_link_libraries(run_avr simavr)
install(TARGETS run_avr RUNTIME DESTINATION bin)

####################################################################################################
#
# End
#
####################################################################################################

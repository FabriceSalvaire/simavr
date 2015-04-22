# -*- CMake -*-

#################################################################################

# Fixme: run test

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  )

file(GLOB TEST_EXECUTABLE_SOURCES test_*.c)

foreach(EXECUTABLE_SRC ${TEST_EXECUTABLE_SOURCES})
  get_filename_component(EXECUTABLE ${EXECUTABLE_SRC} NAME_WE)
  add_executable(${EXECUTABLE} ${EXECUTABLE_SRC} tests.c)
  target_link_libraries(${EXECUTABLE} simavr)
  add_test(${EXECUTABLE} ${EXECUTABLE})
endforeach(EXECUTABLE_SRC)

file(GLOB FIRMWARE_SOURCES at*.c)
foreach(EXECUTABLE_SRC ${FIRMWARE_SOURCES})
  get_filename_component(FILENAME ${EXECUTABLE_SRC} NAME_WE)
  string(REGEX REPLACE "_.*$" "" PART ${FILENAME})
  string(CONCAT FIRMWARE ${FILENAME} ".axf")
  list(APPEND FIRMWARES ${FIRMWARE})
  add_custom_command(OUTPUT ${FIRMWARE} COMMAND
    ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=${PART}
    -I${CMAKE_SOURCE_DIR}/simavr/include/avr
    ${EXECUTABLE_SRC} -o ${FIRMWARE}
    MAIN_DEPENDENCY ${EXECUTABLE_SRC})
endforeach(EXECUTABLE_SRC)

add_custom_target(tests_firmwares ALL
    DEPENDS ${FIRMWARES})

####################################################################################################
#
# End
#
####################################################################################################

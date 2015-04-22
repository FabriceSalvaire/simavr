# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  )

add_executable(analog-in-out-serial
  analog-in-out-serial.c
  potentiometer.c
  )
target_link_libraries(analog-in-out-serial simavr pthread)

####################################################################################################
#
# End
#
####################################################################################################

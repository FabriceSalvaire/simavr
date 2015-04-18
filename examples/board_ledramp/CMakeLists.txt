# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  ${CMAKE_SOURCE_DIR}/examples/parts
  )

add_custom_command(OUTPUT atmega48_ledramp.axf COMMAND
  ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=atmega48
  -I${CMAKE_SOURCE_DIR}/simavr/include/avr
  ${CMAKE_CURRENT_SOURCE_DIR}/atmega48_ledramp.c -o atmega48_ledramp.axf
  MAIN_DEPENDENCY atmega48_ledramp.c)
add_custom_target(ledramp.axf ALL DEPENDS atmega48_ledramp.axf) # Fixme

add_executable(ledramp-gl ledramp-gl.c ../parts/button.c)
target_link_libraries(ledramp-gl simavr GL glut pthread)

add_executable(ledramp ledramp.c ../parts/button.c)
target_link_libraries(ledramp simavr pthread)

####################################################################################################
#
# End
#
####################################################################################################

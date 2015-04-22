# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/simavr/include
  ${CMAKE_SOURCE_DIR}/examples/parts
  ${CMAKE_SOURCE_DIR}/examples/parts/lcd
  )

add_custom_command(OUTPUT atmega48_charlcd.axf COMMAND
  ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=atmega48
  -I${CMAKE_SOURCE_DIR}/simavr/include/avr
  ${CMAKE_CURRENT_SOURCE_DIR}/atmega48_charlcd.c -o atmega48_charlcd.axf
  MAIN_DEPENDENCY atmega48_charlcd.c)
add_custom_target(charlcd.axf ALL DEPENDS atmega48_charlcd.axf) # Fixme

add_executable(charlcd charlcd.c
  ${CMAKE_SOURCE_DIR}/examples/parts/lcd/hd44780.c
  ${CMAKE_SOURCE_DIR}/examples/parts/lcd/hd44780_glut.c
  ${CMAKE_SOURCE_DIR}/examples/parts/ac_input.c
  )
target_link_libraries(charlcd simavr GL glut pthread)

####################################################################################################
#
# End
#
####################################################################################################

# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  ${CMAKE_SOURCE_DIR}/examples/parts
  ${CMAKE_SOURCE_DIR}/examples/parts/bus
  )

add_custom_command(OUTPUT atmega328p_dummy_blinky.axf COMMAND
  ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=atmega48
  -I${CMAKE_SOURCE_DIR}/simavr/include/avr
  ${CMAKE_CURRENT_SOURCE_DIR}/atmega328p_dummy_blinky.c -o atmega328p_dummy_blinky.axf
  MAIN_DEPENDENCY atmega328p_dummy_blinky.c)
add_custom_command(OUTPUT atmega328p_dummy_blinky.hex COMMAND
  avr-objcopy -j .text -j .data -j .eeprom -O ihex
  atmega328p_dummy_blinky.axf atmega328p_dummy_blinky.hex
  MAIN_DEPENDENCY atmega328p_dummy_blinky.axf)
add_custom_target(simduino.axf ALL DEPENDS atmega328p_dummy_blinky.hex) # Fixme

add_executable(simduino simduino.c
  ${CMAKE_SOURCE_DIR}/examples/parts/button.c
  ${CMAKE_SOURCE_DIR}/examples/parts/bus/uart_pty.c)
target_link_libraries(simduino simavr GL glut pthread util)

####################################################################################################
#
# End
#
####################################################################################################

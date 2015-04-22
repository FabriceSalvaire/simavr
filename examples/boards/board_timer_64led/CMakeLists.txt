# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  ${CMAKE_SOURCE_DIR}/examples/parts
  ${CMAKE_SOURCE_DIR}/examples/parts/lcd
  )

add_custom_command(OUTPUT atmega168_timer_64led.axf COMMAND
  ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=atmega168
  -I${CMAKE_SOURCE_DIR}/simavr/include/avr
  ${CMAKE_CURRENT_SOURCE_DIR}/atmega168_timer_64led.c -o atmega168_timer_64led.axf
  MAIN_DEPENDENCY atmega168_timer_64led.c)
add_custom_target(timer_64led.axf ALL DEPENDS atmega168_timer_64led.axf) # Fixme

add_executable(timer_64led timer_64led.c
  ${CMAKE_SOURCE_DIR}/examples/parts/hc595.c
  ${CMAKE_SOURCE_DIR}/examples/parts/button.c
  )
target_link_libraries(timer_64led simavr GL glut pthread)

####################################################################################################
#
# End
#
####################################################################################################

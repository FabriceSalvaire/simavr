# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  )

add_custom_command(OUTPUT atmega88_uart_echo.axf COMMAND
  ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=atmega48
  -I${CMAKE_SOURCE_DIR}/simavr/include/avr
  ${CMAKE_CURRENT_SOURCE_DIR}/atmega88_uart_echo.c -o atmega88_uart_echo.axf
  MAIN_DEPENDENCY atmega88_uart_echo.c)
add_custom_target(atmega88_uart_echo.axf ALL DEPENDS atmega88_uart_echo.axf) # Fixme

add_executable(uart uart.c)
target_link_libraries(uart simavr pthread)

####################################################################################################
#
# End
#
####################################################################################################

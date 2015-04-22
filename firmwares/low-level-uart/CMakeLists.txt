####################################################################################################

# generate_avr_firmware(atmega88_uart_echo
#   BOARD ???
#   SRCS atmega88_uart_echo.c)

generate_avr_firmware(atmega2560_uart_echo
  BOARD mega2560
  SRCS atmega2560_uart_echo.c)

install(TARGETS
  # atmega88_uart_echo
  atmega2560_uart_echo
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)
install(FILES
  # ${CMAKE_CURRENT_BINARY_DIR}/atmega88_uart_echo.hex
  ${CMAKE_CURRENT_BINARY_DIR}/atmega2560_uart_echo.hex
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)

####################################################################################################
#
# End
#
####################################################################################################

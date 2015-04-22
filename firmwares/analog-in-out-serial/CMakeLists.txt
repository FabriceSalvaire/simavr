####################################################################################################

generate_arduino_firmware(analog-in-out-serial
  BOARD mega2560
  SKETCH analog-in-out-serial.c)

install(TARGETS analog-in-out-serial
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/analog-in-out-serial.hex
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)

####################################################################################################
#
# End
#
####################################################################################################

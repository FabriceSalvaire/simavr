####################################################################################################

generate_arduino_firmware(fade-led
  BOARD mega2560
  SKETCH fade-led.c)

install(TARGETS fade-led
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fade-led.hex
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)

####################################################################################################
#
# End
#
####################################################################################################

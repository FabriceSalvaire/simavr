####################################################################################################

generate_arduino_firmware(blink-led
  SKETCH blink-led.c)

install(TARGETS blink-led
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/blink-led.hex
  DESTINATION ${FIRMWARES_BINARY_DIR})
# DESTINATION share/simavr/firmwares)

####################################################################################################
#
# End
#
####################################################################################################

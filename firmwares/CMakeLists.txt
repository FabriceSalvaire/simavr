####################################################################################################

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ArduinoToolchain.cmake) # Arduino Toolchain

cmake_minimum_required(VERSION 2.8)

project(Firmwares C CXX)

####################################################################################################

set(ARDUINO_DEFAULT_BOARD mega2560)

set(FIRMWARES_BINARY_DIR ${SIMAVR_BINARY_DIR}/firmwares)

# print_board_list()

include_directories(
  ${CMAKE_SOURCE_DIR}/../simavr/include/avr
  )

####################################################################################################

# add_subdirectory(analog-read-serial)
# add_subdirectory(digital-read-serial)
# add_subdirectory(read-analog-voltage)
add_subdirectory(analog-in-out-serial)
add_subdirectory(blink-led)
add_subdirectory(fade-led)
add_subdirectory(low-level-uart)

####################################################################################################
#
# End
#
####################################################################################################

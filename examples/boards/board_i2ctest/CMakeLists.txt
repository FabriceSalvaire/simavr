# -*- CMake -*-

#################################################################################

include_directories(
  ${CMAKE_SOURCE_DIR}/simavr/include
  ${CMAKE_SOURCE_DIR}/examples/parts
  )

add_custom_command(OUTPUT atmega1280_i2ctest.axf COMMAND
  ${AVR_GCC} ${AVR_GCC_CFLAGS} -mmcu=atmega1280
  -I${CMAKE_SOURCE_DIR}/simavr/include/avr
  -I${CMAKE_SOURCE_DIR}/firmwares
  ${CMAKE_CURRENT_SOURCE_DIR}/atmega1280_i2ctest.c
  ${CMAKE_SOURCE_DIR}/firmwares/twi/twimaster.c
  ${CMAKE_SOURCE_DIR}/firmwares/twi/avr_twi_master.c
  -o atmega1280_i2ctest.axf
  MAIN_DEPENDENCY atmega1280_i2ctest.c
  )
add_custom_target(i2ctest.axf ALL DEPENDS atmega1280_i2ctest.axf) # Fixme

add_executable(i2ctest i2ctest.c
  ${CMAKE_SOURCE_DIR}/examples/parts/i2c_eeprom.c
  )
target_link_libraries(i2ctest simavr GL glut pthread)

####################################################################################################
#
# End
#
####################################################################################################

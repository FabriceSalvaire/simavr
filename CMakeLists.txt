# -*- CMake -*-

####################################################################################################
#
# Main CMakeLists.txt to build SimAVR using CMake (www.cmake.org)
# Written by Fabrice Salvaire
#
####################################################################################################

####################################################################################################
#
# Customizable settings
#

# -D <var>:<type>=<value>: Create a cmake cache entry.

# Install path prefix, prepended onto install directories.
# -D CMAKE_INSTALL_PREFIX:PATH=/usr/local/

# Choose the type of build, options are: None(CMAKE_CXX_FLAGS or
# CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.
# -D CMAKE_BUILD_TYPE:STRING=Debug

# If this value is on, makefiles will be generated without the
# .SILENT directive, and all commands will be echoed to the console
# during the make.  This is useful for debugging only.
# -D CMAKE_VERBOSE_MAKEFILE:BOOL=ON

####################################################################################################

project(simavr C)

set(SIMAVR_VERSION_MAJOR 1)
set(SIMAVR_VERSION_MINOR 0)
set(SIMAVR_VERSION_PATCH 0) # right name?
set(SIMAVR_VERSION ${SIMAVR_VERSION_MAJOR}.${SIMAVR_VERSION_MINOR}.${SIMAVR_VERSION_PATCH})

####################################################################################################
#
# CMake Configuration
#

# check cmake version
cmake_minimum_required(VERSION 3.0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
# set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

####################################################################################################
#
# Options
#

set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

####################################################################################################
#
# Compiler Options
#

set(CMAKE_C_FLAGS "-Wall -fPIC --std=gnu99 -MMD -O2") # -g

# if(CMAKE_SYSTEM_NAME MATCHES "Linux")
# endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

####################################################################################################
#
# Find Libraries
#

find_program(AVR_GCC avr-gcc)
find_path(AVR_INCLUDE_DIRECTORY avr/version.h /usr/avr/include/)
set(AVR_GCC_CFLAGS
  -Wall -gdwarf-2 -Os -std=gnu99
  -DF_CPU=8000000
  -fno-inline-small-functions
  -ffunction-sections -fdata-sections
  -Wl,--relax,--gc-sections
  -Wl,--undefined=_mmcu,--section-start=.mmcu=0x910000
  )

# find_package()
# find_library()
  
####################################################################################################
#
# Include Directories
#

# include_directories()

####################################################################################################
#
# Sub directories
#

add_subdirectory(simavr)
add_subdirectory(tests)
add_subdirectory(examples)

####################################################################################################
#
# Install
#

install(FILES
  README.md
  COPYING
  DESTINATION share/doc/simavr
  )

####################################################################################################
#
# End
#
####################################################################################################

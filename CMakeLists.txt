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

enable_testing()

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

# -Wpedantic: Issue all the warnings demanded by strict ISO C 
# -Wall: This enables all the warnings about constructions that some users consider questionable
# -Wextra: This enables some extra warning flags that are not enabled by -Wall

set(C_FLAGS_WARNING
  -Wall
  -Wextra
  -Wno-unused-parameter
  )
set(C_FLAGS_STANDARD --std=c99 -Wpedantic)
set(C_FLAGS_ERROR -fdiagnostics-color) # colourise GCC output

set(CMAKE_C_FLAGS "${C_FLAGS_WARNING} ${C_FLAGS_STANDARD} ${C_FLAGS_ERROR} -O3") # -g

# if(CMAKE_SYSTEM_NAME MATCHES "Linux")
# endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

####################################################################################################
#
# Find Libraries
#

# find_package()
# find_library()

####################################################################################################

# AVR GCC Compiler Flags
# 
# -gdwarf-version: Produce debugging information in DWARF format
# 
# Use short types
#   -funsigned-char
#   -funsigned-bitfields
#   -fpack-struct
#   -fshort-enums
# 
# Set the cost of inline calls
#   --param inline-call-cost=2
#   -finline-limit=3
#   -fno-inline-small-functions
# 
#   Prevents or limits the compiler from automatically inlining small functions automatically for
#   speed. Doing so will usually make the program faster, but if the routine is used a lot you'll
#   end up with a larger FLASH usage.
# 
# Don't include unused function and data
#   -ffunction-sections
#   -fdata-sections
#   
#   Generally used with --gc-sections. This causes each function to be placed into a separate
#   internal memory section, which --gc-sections can then discard if the section (function) is
#   unreferenced. Those two together will only help if you have a bunch of unused functions in your
#   code, such as when you are using a library compiled with the option and linked statically.
#   
#   Note that if you have any "naked" functions in the placed in an .initN or .finiN section, you
#   should also mark the function with a "used" attribute to prevent it's being optimised
#   out. (Thanks to David Boone.)
# 
# Compile a freestanding program
#   -ffreestanding and void main() __attribute__ ((noreturn));
# 
#   Tell the compiler your main function is not returning. This saves some bytes on the stack.
# 
# Linker Relaxation
#    -Wl,--relax
# 
#    Almost always a win. Note that this is a linker flag that is passed to the linker using the -Wl
#    flag.  Enables linker relaxations. By default, the linker links functions will a full CALL
#    statement, which is wasteful if two functions are near each other. Relaxations will do more in
#    the future, but currently (AFAIK) just replace CALL statements with RCALL where possible to
#    save a few bytes.
# 
# Call Prologues/Epilogues
#   -mcall-prologues
# 
#   Your application must be large enough for this to be a win. Take a close look at the code size
#   before and after to make sure that this will actually decrease your code size.
# 
# Whole Program Optimisation
#   --combine-fwhole-program
# 
#   Using these two flags will turn on whole program optimisation, but can only be done on C code
#   (which is typical for an AVR). The catch is that you have to modify your Makefile to have it
#   execute only a single command-line call to avr-gcc with *all* of your C files and all compiler
#   and linker flags. GCC will then combine, compile, and link all the code into the final ELF file,
#   but this also allows it to do whole program optimisation.
# 
# Loop optimisation
#   -fno-tree-scev-cprop
# 
#   It will vectorise the outer loop when multiple loops are nested for a size benefit.
# 
# Wide types
#   -fno-split-wide-types
# 
#   When using a type that occupies multiple registers, such as long long on a 32-bit system, split
#   the registers apart and allocate them independently. This normally generates better code for
#   those types, but may make debugging more difficult. The option -fno-split-wide-types will stop
#   this.
 
find_program(AVR_GCC avr-gcc)
find_path(AVR_INCLUDE_DIRECTORY avr/version.h /usr/avr/include/)
set(AVR_GCC_CFLAGS
  -DF_CPU=8000000
  -Wall
  -std=c99
  -gdwarf-2
  -Os
  -fno-inline-small-functions
  -ffunction-sections -fdata-sections
  -Wl,--relax,--gc-sections
  -Wl,--undefined=_mmcu,--section-start=.mmcu=0x910000
  )
  
####################################################################################################
#
# Include Directories
#

# include_directories()

####################################################################################################

# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmwares)
# execute_process(
#   COMMAND cmake ${CMAKE_CURRENT_SOURCE_DIR}/firmwares
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmwares)
# execute_process(
#   COMMAND make
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmwares)

# drawback: make once
include(ExternalProject)
ExternalProject_add(firmwares
  PREFIX ${CMAKE_BINARY_DIR}/firmwares-project
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/firmwares
  CMAKE_ARGS -DSIMAVR_BINARY_DIR:PATH=${CMAKE_BINARY_DIR}
  # BUILD_ALWAYS 1 # purpse ?
  # INSTALL_COMMAND "" # hack to disable the install
  )

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

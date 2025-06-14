# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/radu-tolontan/esp/v5.4.1/esp-idf/components/bootloader/subproject"
  "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader"
  "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix"
  "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix/tmp"
  "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix/src/bootloader-stamp"
  "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix/src"
  "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/radu-tolontan/code/BASM_card/SW_design/PlatformIO_C6/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

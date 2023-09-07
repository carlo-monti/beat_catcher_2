# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/carlomonti/esp/esp-idf/components/bootloader/subproject"
  "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader"
  "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix"
  "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix/tmp"
  "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix/src"
  "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/carlomonti/BeatCatcher2/MAIN_APP_2/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

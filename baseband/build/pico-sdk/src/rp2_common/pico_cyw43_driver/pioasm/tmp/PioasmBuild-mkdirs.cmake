# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/wenya589/TA_courses/wcnes_project/2023/pico/pico-sdk/tools/pioasm"
  "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pioasm"
  "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm"
  "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
  "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/wenya589/mana/Github Repos/Pico-Backscatter/baseband/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()

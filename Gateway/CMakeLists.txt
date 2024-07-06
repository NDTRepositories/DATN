cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/example/common_components/protocol_examples_common)
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)
project(Gateway)

spiffs_create_partition_image(storage0 font FLASH_IN_PROJECT)
spiffs_create_partition_image(storage1 icons FLASH_IN_PROJECT)
spiffs_create_partition_image(storage2 images FLASH_IN_PROJECT)
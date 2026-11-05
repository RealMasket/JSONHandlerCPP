if(NOT CPACK_BUILD_CONFIG STREQUAL "Release")
    message(FATAL_ERROR "Release archives must be packaged with cpack -C Release")
endif()

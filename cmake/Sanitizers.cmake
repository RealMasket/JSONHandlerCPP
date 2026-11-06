if(MASKETJSON_ENABLE_SANITIZERS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
        NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        message(FATAL_ERROR "MASKETJSON_ENABLE_SANITIZERS requires Linux with GCC or Clang")
    endif()
    add_library(MasketJSONSanitizers INTERFACE)
    target_compile_options(MasketJSONSanitizers INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(MasketJSONSanitizers INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=all)
    # Instrument the library AND each consumer's own source, and link runtimes
    # through the compiler driver rather than invoking the linker directly.
    target_link_libraries(MasketJSON PUBLIC MasketJSONSanitizers)
endif()

if(MASKETJSON_BUILD_FUZZERS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
        NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR BUILD_SHARED_LIBS OR
        NOT MASKETJSON_ENABLE_SANITIZERS)
        message(FATAL_ERROR
            "Fuzzers require Linux Clang, BUILD_SHARED_LIBS=OFF and MASKETJSON_ENABLE_SANITIZERS=ON")
    endif()
endif()

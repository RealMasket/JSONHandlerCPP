if(JSONHANDLER_ENABLE_SANITIZERS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
        NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
        message(FATAL_ERROR "JSONHANDLER_ENABLE_SANITIZERS requires Linux with GCC or Clang")
    endif()
    add_library(JsonHandlerSanitizers INTERFACE)
    target_compile_options(JsonHandlerSanitizers INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(JsonHandlerSanitizers INTERFACE
        -fsanitize=address,undefined -fno-sanitize-recover=all)
    # Instrument the library AND each consumer's own source, and link runtimes
    # through the compiler driver rather than invoking the linker directly.
    target_link_libraries(JsonHandlerCPP PUBLIC JsonHandlerSanitizers)
endif()

if(JSONHANDLER_BUILD_FUZZERS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
        NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR BUILD_SHARED_LIBS OR
        NOT JSONHANDLER_ENABLE_SANITIZERS)
        message(FATAL_ERROR
            "Fuzzers require Linux Clang, BUILD_SHARED_LIBS=OFF and JSONHANDLER_ENABLE_SANITIZERS=ON")
    endif()
endif()

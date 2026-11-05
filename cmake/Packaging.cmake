# Release archives contain only files from this project's install rules.
if(JSONHANDLER_ENABLE_SANITIZERS OR JSONHANDLER_BUILD_FUZZERS)
    message(FATAL_ERROR "Release packaging requires an unsanitized build")
endif()
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "Release packaging requires CMAKE_BUILD_TYPE=Release")
endif()
set(CPACK_PACKAGE_NAME JsonHandlerCPP)
set(CPACK_PACKAGE_VENDOR RealMasket)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++17 JSON library and command-line validator/formatter")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/RealMasket/JSONHandlerCPP")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/packages")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_SOURCE_DIR}/cmake/CPackRelease.cmake")
# Source releases use git archive so untracked local files cannot be included.
set(CPACK_SOURCE_GENERATOR "")

if(BUILD_SHARED_LIBS)
    set(jsonhandler_linkage shared)
else()
    set(jsonhandler_linkage static)
endif()
if(CMAKE_GENERATOR_PLATFORM)
    set(jsonhandler_arch "${CMAKE_GENERATOR_PLATFORM}")
else()
    set(jsonhandler_arch "${CMAKE_SYSTEM_PROCESSOR}")
endif()
if(WIN32)
    set(CPACK_GENERATOR ZIP)
    set(jsonhandler_archive_extension zip)
else()
    set(CPACK_GENERATOR TGZ)
    set(jsonhandler_archive_extension tar.gz)
endif()
set(CPACK_PACKAGE_FILE_NAME
    "JsonHandlerCPP-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${jsonhandler_arch}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-${jsonhandler_linkage}")
string(REGEX REPLACE "[^A-Za-z0-9_.-]" "_" CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
# CPack also prepares source settings and may change CPACK_PACKAGE_FILE_NAME.
set(jsonhandler_archive_basename "${CPACK_PACKAGE_FILE_NAME}")
include(CPack)

if(BUILD_TESTING)
    add_test(NAME Package.ArchiveConsumer COMMAND "${CMAKE_COMMAND}"
        ${jsonhandler_consumer_test_args}
        "-DCPACK_COMMAND=${CMAKE_CPACK_COMMAND}"
        "-DCPACK_CONFIG=${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake"
        "-DPACKAGE_OUTPUT_DIR=${CPACK_PACKAGE_DIRECTORY}"
        "-DARCHIVE_BASENAME=${jsonhandler_archive_basename}"
        "-DARCHIVE_EXTENSION=${jsonhandler_archive_extension}"
        -P "${PROJECT_SOURCE_DIR}/tests/InstalledPackage.cmake")
    set_tests_properties(Package.ArchiveConsumer PROPERTIES TIMEOUT 180)
endif()

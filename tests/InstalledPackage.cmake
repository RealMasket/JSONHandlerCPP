foreach(required IN ITEMS PRODUCER_BINARY_DIR PRODUCER_SOURCE_DIR CONSUMER_SOURCE_DIR
    TEST_ROOT GENERATOR CXX_COMPILER INSTALL_LIBDIR INSTALL_BINDIR INSTALL_INCLUDEDIR
    INSTALL_DOCDIR CTEST_COMMAND)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()

function(run_checked)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result
        OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT "${result}" STREQUAL "0")
        string(JOIN " " command ${ARGV})
        message(FATAL_ERROR "Command failed (${result}): ${command}\n${output}\n${error}")
    endif()
endfunction()

# Never clear a caller-supplied directory. Each run owns a new child directory,
# and relocation is restricted to the two prefixes created inside that child.
cmake_path(ABSOLUTE_PATH TEST_ROOT NORMALIZE OUTPUT_VARIABLE test_root)
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef unique_id)
set(work "${test_root}/run ${CONFIG}-${unique_id}")
set(original "${work}/prefix original")
set(relocated "${work}/prefix relocated")
cmake_path(IS_PREFIX test_root "${work}" NORMALIZE work_inside_root)
cmake_path(IS_PREFIX work "${original}" NORMALIZE original_inside_work)
cmake_path(IS_PREFIX work "${relocated}" NORMALIZE relocated_inside_work)
if(NOT work_inside_root OR NOT original_inside_work OR NOT relocated_inside_work)
    message(FATAL_ERROR "Install/relocation paths escape the test workspace")
endif()
foreach(directory IN ITEMS "${INSTALL_LIBDIR}" "${INSTALL_BINDIR}" "${INSTALL_INCLUDEDIR}" "${INSTALL_DOCDIR}")
    cmake_path(IS_PREFIX original "${original}/${directory}" NORMALIZE inside_prefix)
    if(IS_ABSOLUTE "${directory}" OR NOT inside_prefix)
        message(FATAL_ERROR "Relocation test requires relative install directories inside its prefix: ${directory}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${work}")
run_checked("${CMAKE_COMMAND}" --install "${PRODUCER_BINARY_DIR}"
    --config "${CONFIG}" --prefix "${original}")
if(NOT IS_DIRECTORY "${original}")
    message(FATAL_ERROR "Installation did not create the requested prefix")
endif()
file(RENAME "${original}" "${relocated}")

# Exported metadata must not silently refer to producer source/build trees.
file(TO_CMAKE_PATH "${PRODUCER_SOURCE_DIR}" producer_source)
file(TO_CMAKE_PATH "${PRODUCER_BINARY_DIR}" producer_build)
file(GLOB package_files "${relocated}/${INSTALL_LIBDIR}/cmake/JsonHandlerCPP/*.cmake")
if(NOT package_files)
    message(FATAL_ERROR "Installed package configuration is missing")
endif()
foreach(package_file IN LISTS package_files)
    file(READ "${package_file}" contents)
    foreach(forbidden IN ITEMS "${producer_source}" "${producer_build}")
        string(FIND "${contents}" "${forbidden}" location)
        if(NOT location EQUAL -1)
            message(FATAL_ERROR "Producer path leaked into installed config: ${package_file}")
        endif()
    endforeach()
endforeach()

set(consumer_source "${work}/consumer-source")
set(consumer_build "${work}/consumer-build")
file(COPY "${CONSUMER_SOURCE_DIR}/" DESTINATION "${consumer_source}")
set(configure_args -S "${consumer_source}" -B "${consumer_build}" -G "${GENERATOR}"
    "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}"
    "-DCMAKE_BUILD_TYPE=${CONFIG}"
    "-DCMAKE_PREFIX_PATH=${relocated}"
    "-DEXPECT_PREFIX=${relocated}"
    "-DEXPECT_SHARED=${EXPECT_SHARED}"
    -DBUILD_TESTING=ON
    -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF
    -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF)
if(DEFINED GENERATOR_PLATFORM AND NOT GENERATOR_PLATFORM STREQUAL "")
    list(APPEND configure_args -A "${GENERATOR_PLATFORM}")
endif()
if(DEFINED GENERATOR_TOOLSET AND NOT GENERATOR_TOOLSET STREQUAL "")
    list(APPEND configure_args -T "${GENERATOR_TOOLSET}")
endif()
run_checked("${CMAKE_COMMAND}" ${configure_args})
run_checked("${CMAKE_COMMAND}" --build "${consumer_build}" --config "${CONFIG}" --parallel 2)
run_checked("${CTEST_COMMAND}" --test-dir "${consumer_build}" -C "${CONFIG}" --output-on-failure)

if(CLI_ENABLED)
    set(cli "${relocated}/${INSTALL_BINDIR}/jsonhandler${EXECUTABLE_SUFFIX}")
    execute_process(COMMAND "${cli}" format --compact "${consumer_source}/input.json"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    string(REPLACE "\r\n" "\n" output "${output}")
    if(NOT "${result}" STREQUAL "0" OR NOT output STREQUAL "[18446744073709551615,\"001\",-0.0]\n")
        message(FATAL_ERROR "Relocated installed CLI failed (${result}): ${output}\n${error}")
    endif()
endif()
message(STATUS "Relocated package and independent consumer passed: ${relocated}")

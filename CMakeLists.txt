# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.28)

project("booru-sync")

macro(setup_target)
    cmake_parse_arguments(SETUP_TARGET "" "TARGET" "LIBRARIES" ${ARGN})

    target_compile_definitions(${SETUP_TARGET_TARGET} PRIVATE _CRT_SECURE_NO_WARNINGS)

    set_target_properties(${SETUP_TARGET_TARGET} PROPERTIES
        # No actual C++26 support in CMAKE
        # CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )

    target_include_directories(${SETUP_TARGET_TARGET} PRIVATE "${TCLAP_INCLUDE_DIRS}")

    if (MSVC)
        target_compile_options(${SETUP_TARGET_TARGET} PRIVATE /W3)
    else()
        target_compile_options(${SETUP_TARGET_TARGET} PRIVATE -Wall -Wextra -Wpedantic -std=c++26)
    endif()

    target_link_libraries(${SETUP_TARGET_TARGET} PRIVATE ${SETUP_TARGET_LIBRARIES})

    set_target_properties(${SETUP_TARGET_TARGET}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endmacro()

find_path(TCLAP_INCLUDE_DIRS "tclap/Arg.h")
find_package(cpr CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

add_subdirectory("src")

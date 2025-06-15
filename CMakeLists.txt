cmake_minimum_required(VERSION 3.18)

project(AcceleratorCSS C CXX ASM)

include("cmake/shared.cmake")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/funchook)

set_property(TARGET funchook-static PROPERTY POSITION_INDEPENDENT_CODE ON)

set(HL2SDK vendor/hl2sdk-cs2)
set(METAMOD vendor/metamod-source)

include("cmake/protobuf.cmake")
include("cmake/breakpad.cmake")

set(SOURCE_FILES
        ${HL2SDK}/public/tier0/memoverride.cpp
        ${HL2SDK}/tier1/convar.cpp
        ${HL2SDK}/tier1/generichash.cpp
        ${HL2SDK}/tier1/keyvalues3.cpp
        ${HL2SDK}/entity2/entityidentity.cpp
        ${HL2SDK}/entity2/entitysystem.cpp
        ${HL2SDK}/entity2/entitykeyvalues.cpp

        ${METAMOD}/core/sourcehook/sourcehook.cpp
        ${METAMOD}/core/sourcehook/sourcehook_impl_chookidman.cpp
        ${METAMOD}/core/sourcehook/sourcehook_impl_chookmaninfo.cpp
        ${METAMOD}/core/sourcehook/sourcehook_impl_cvfnptr.cpp
        ${METAMOD}/core/sourcehook/sourcehook_impl_cproto.cpp

        src/extension.cpp
        src/extension.h
        src/CMiniDumpComment.hpp
        src/log.cpp
        src/log.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${NATIVES_SOURCES} ${CONVERSIONS_SOURCES} ${CONVERSIONS_HEADERS})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include("cmake/linux.base.cmake")

set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/addons/AcceleratorCSS/bin/linuxsteamrt64"
)

set_source_files_properties(${BREAKPAD_CLIENT_SOURCES} PROPERTIES LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME}
        ${ACCELERATORCSS_LINK_LIBRARIES}
        Protobufs
        breakpad
        breakpad-client
)

add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/configs ${CMAKE_BINARY_DIR}
)

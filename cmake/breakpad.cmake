# Collect relevant Linux sources for Breakpad
file(GLOB_RECURSE BREAKPAD_CLIENT_SOURCES
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src/client/linux/*.cc
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src/common/*.cc
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src/common/linux/*.cc
)

# Remove unit tests, unsupported platforms, and tools we don't need
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*_unittest\\.cc$")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*googletest.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*/tests/.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*/(mac|ios|windows|solaris)/.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*breakpad_nlist_64.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*dump_symbols.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*google_crashdump_uploader_test.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*dwarf_cu_to_module.*")
list(FILTER BREAKPAD_CLIENT_SOURCES EXCLUDE REGEX ".*language\\.cc")

# Optional: print remaining sources
# foreach(src ${BREAKPAD_CLIENT_SOURCES})
#     message(STATUS "Breakpad source: ${src}")
# endforeach()

add_library(breakpad-client STATIC ${BREAKPAD_CLIENT_SOURCES})

target_include_directories(breakpad-client PUBLIC
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src/common
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src/client/linux
        ${CMAKE_SOURCE_DIR}/vendor/breakpad/src/google_breakpad
)

target_compile_definitions(breakpad-client PRIVATE
        _LINUX
        LINUX
        HAVE_A_OUT_H
)

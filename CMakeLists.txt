cmake_minimum_required(VERSION 3.12)
project(fbltrimzero VERSION 1.0.0 LANGUAGES CXX)

# =============================================================================
# Build options
# =============================================================================
option(FIREBIRD_INCLUDE_DIR "Path to Firebird include directory" "")
option(BUILD_SHARED_LIBS "Build shared library" ON)

# =============================================================================
# Compiler settings
# =============================================================================
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Visual Studio
    add_compile_options(/W3 /EHsc /MD)
else()
    # GCC/Clang
    add_compile_options(-Wall -fPIC)
endif()

# =============================================================================
# Find Firebird headers (optional - we have standalone version)
# =============================================================================
if(FIREBIRD_INCLUDE_DIR)
    message(STATUS "Using Firebird headers from: ${FIREBIRD_INCLUDE_DIR}")
    include_directories(${FIREBIRD_INCLUDE_DIR})
    add_compile_definitions(USE_FIREBIRD_HEADERS)
else()
    message(STATUS "Using standalone headers (embedded in source)")
endif()

# =============================================================================
# Source files
# =============================================================================
set(SOURCES
    src/fbltrimzero.cpp
)

# =============================================================================
# Build library
# =============================================================================
add_library(fbltrimzero MODULE ${SOURCES})

# Set output name (without 'lib' prefix on Unix)
set_target_properties(fbltrimzero PROPERTIES
    PREFIX ""
    OUTPUT_NAME "fbltrimzero"
)

# =============================================================================
# Installation
# =============================================================================
if(WIN32)
    set(FIREBIRD_PLUGIN_DIR "C:/Program Files/Firebird/Firebird_5_0/intl" CACHE PATH "Firebird intl directory")
else()
    set(FIREBIRD_PLUGIN_DIR "/opt/firebird/intl" CACHE PATH "Firebird intl directory")
endif()

install(TARGETS fbltrimzero
    LIBRARY DESTINATION ${FIREBIRD_PLUGIN_DIR}
    RUNTIME DESTINATION ${FIREBIRD_PLUGIN_DIR}
)

install(FILES config/fbltrimzero.conf
    DESTINATION ${FIREBIRD_PLUGIN_DIR}
)

# =============================================================================
# Package configuration
# =============================================================================
set(CPACK_PACKAGE_NAME "fbltrimzero")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Firebird LTRIM_ZERO Collation Plugin")
set(CPACK_PACKAGE_VENDOR "Community")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

# =============================================================================
# Print summary
# =============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  fbltrimzero Configuration Summary")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Plugin directory: ${FIREBIRD_PLUGIN_DIR}")
message(STATUS "========================================")
message(STATUS "")

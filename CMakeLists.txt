cmake_minimum_required(VERSION 3.15)
project(obs-vtp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find libobs and obs-frontend-api
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBOBS REQUIRED libobs)
pkg_check_modules(OBS_FRONTEND REQUIRED obs-frontend-api)

# Find VTP (Assuming VTP headers and library are placed in the sdk folder)
set(VTP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include")
set(VTP_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/sdk/lib/libvtp.so")

if(NOT EXISTS "${VTP_LIBRARY}")
    message(WARNING "VTP Library not found at ${VTP_LIBRARY}. Please copy libvtp.so and vtp.h into the 'sdk' directory before building.")
endif()

# Plugin target
add_library(obs-vtp MODULE
    src/obs-vtp.cpp
    src/vtp-source.cpp
    src/vtp-output.cpp
)

target_include_directories(obs-vtp PRIVATE
    src
    ${LIBOBS_INCLUDE_DIRS}
    ${LIBOBS_INCLUDE_DIRS}/obs
    ${OBS_FRONTEND_INCLUDE_DIRS}
    ${VTP_INCLUDE_DIR}
)

target_link_libraries(obs-vtp PRIVATE
    ${LIBOBS_LIBRARIES}
    ${OBS_FRONTEND_LIBRARIES}
    ${VTP_LIBRARY}
)

# Output naming convention for OBS plugins on Linux and RPATH setup
set_target_properties(obs-vtp PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-vtp"
    INSTALL_RPATH "$ORIGIN"
)

# Installation instruction (standard OBS plugin path on Linux)
install(TARGETS obs-vtp DESTINATION ~/.config/obs-studio/plugins/obs-vtp/bin/64bit)
install(FILES "${VTP_LIBRARY}" DESTINATION ~/.config/obs-studio/plugins/obs-vtp/bin/64bit)

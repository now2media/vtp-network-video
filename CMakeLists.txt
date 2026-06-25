cmake_minimum_required(VERSION 3.15)
project(vtp-network-video VERSION 1.0.0 LANGUAGES CXX)

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
    message(WARNING "VTP Library not found at ${VTP_LIBRARY}.")
endif()

# Plugin target
add_library(vtp-network-video MODULE
    src/vtp-network-video.cpp
    src/vtp-source.cpp
    src/vtp-output.cpp
)

target_include_directories(vtp-network-video PRIVATE
    src
    ${LIBOBS_INCLUDE_DIRS}
    ${LIBOBS_INCLUDE_DIRS}/obs
    ${OBS_FRONTEND_INCLUDE_DIRS}
    ${VTP_INCLUDE_DIR}
)

target_link_libraries(vtp-network-video PRIVATE
    ${LIBOBS_LIBRARIES}
    ${OBS_FRONTEND_LIBRARIES}
    ${VTP_LIBRARY}
)

# Output naming convention for OBS plugins on Linux and RPATH setup
set_target_properties(vtp-network-video PROPERTIES
    PREFIX ""
    OUTPUT_NAME "vtp-network-video"
    INSTALL_RPATH "$ORIGIN"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/vtp-network-video/bin/64bit"
)

# Copy libvtp.so dependency directly to build/vtp-network-video/bin/64bit after build
add_custom_command(TARGET vtp-network-video POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/vtp-network-video/bin/64bit"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${VTP_LIBRARY}" "${CMAKE_BINARY_DIR}/vtp-network-video/bin/64bit/"
    COMMENT "Copying libvtp.so"
)

# Installation instruction (standard OBS plugin path on Linux)
install(TARGETS vtp-network-video DESTINATION ~/.config/obs-studio/plugins/vtp-network-video/bin/64bit)
install(FILES "${VTP_LIBRARY}" DESTINATION ~/.config/obs-studio/plugins/vtp-network-video/bin/64bit)

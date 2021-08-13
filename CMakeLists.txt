cmake_minimum_required(VERSION 3.0)

project(winspice)

include_directories(include)

find_package(PkgConfig)
pkg_check_modules(SPICE spice-server)
pkg_check_modules(GLIB glib-2.0)
pkg_check_modules(GTK gtk+-3.0)
include_directories(${SPICE_INCLUDEDIR} ${SPICE_INCLUDE_DIRS} ${GLIB_INCLUDEDIR} ${GLIB_INCLUDE_DIRS} ${GTK_INCLUDEDIR} ${GTK_INCLUDE_DIRS})
find_library(SPICE spice-server)

aux_source_directory(src DIR_SRCS)
set(WINSPICE_LIBS d3d11 dxgi dxguid ${SPICE} ${GLIB_LIBRARIES} ${GTK_LIBRARIES})
add_executable(${PROJECT_NAME} ${DIR_SRCS})
target_link_libraries(${PROJECT_NAME} ${WINSPICE_LIBS})
add_compile_options(-Werror -Wall)
target_compile_options(${PROJECT_NAME} PUBLIC -Werror -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter)
target_link_options(${PROJECT_NAME} PUBLIC -Wl,--subsystem,windows)

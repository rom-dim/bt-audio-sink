cmake_minimum_required(VERSION 2.8)

project(bt-audio-sink)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0 )
include_directories(${GLIB_INCLUDE_DIRS})
pkg_check_modules(GIO REQUIRED gio-2.0)

find_package(DBus1 REQUIRED)
include_directories(${DBus1_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} "main.cpp" "MediaControlClass.cpp" "AdapterControlClass.cpp")

target_link_libraries(${PROJECT_NAME} ${GLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${GIO_LIBRARIES})
target_link_libraries(${PROJECT_NAME}  ${DBus1_LIBRARIES})

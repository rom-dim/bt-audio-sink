cmake_minimum_required(VERSION 2.8)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
project(buttons-emu)
add_executable(${PROJECT_NAME} "main-emu.cpp" "gpiolib.c")


project(bt-audio-sink)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0 )
include_directories(${GLIB_INCLUDE_DIRS})
pkg_check_modules(GIO REQUIRED gio-2.0)

add_executable(${PROJECT_NAME} "main.cpp" "MediaControlClass.cpp" "AdapterControlClass.cpp" "DeviceClass.cpp" "CarControl.cpp")

target_link_libraries(${PROJECT_NAME} ${GLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${GIO_LIBRARIES})
target_link_libraries(${PROJECT_NAME} dbus-1)

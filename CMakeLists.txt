cmake_minimum_required(VERSION 3.16)
project(MyClient C)

include(FindPkgConfig)

set(MPLANE_PATH  "/usr/local" CACHE STRING "Enter M-plane path")  # can be in /opt/ directory, if multiple versions exist
set(ENV{PKG_CONFIG_PATH} "${MPLANE_PATH}/lib/pkgconfig")

pkg_check_modules(libnetconf2 REQUIRED libnetconf2)
message(STATUS "${libnetconf2_INCLUDE_DIRS}")
message(STATUS "${libnetconf2_LINK_LIBRARIES}")

pkg_check_modules(libyang REQUIRED libyang)
message(STATUS "${libyang_INCLUDE_DIRS}")
message(STATUS "${libyang_LINK_LIBRARIES}")

add_executable(my_client main.c)

target_include_directories(my_client PRIVATE ${libnetconf2_INCLUDE_DIRS})
target_link_libraries(my_client PRIVATE ${libnetconf2_LINK_LIBRARIES})

target_include_directories(my_client PRIVATE ${libyang_INCLUDE_DIRS})
target_link_libraries(my_client PRIVATE ${libyang_LDFLAGS} ${libyang_LINK_LIBRARIES})

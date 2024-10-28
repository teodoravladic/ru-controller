cmake_minimum_required(VERSION 3.16)
project(MyClient C)

include(FindPkgConfig)

set(MPLANE_PATH  "/usr/local" CACHE STRING "Enter M-plane path")  # can be in /opt/ directory, if multiple versions exist
set(ENV{PKG_CONFIG_PATH} "${MPLANE_PATH}/lib/pkgconfig")

set(MPLANE_VERSION "V1" CACHE STRING "M-plane version")
set_property(CACHE MPLANE_VERSION PROPERTY STRINGS "V1" "V2")

pkg_check_modules(libnetconf2 REQUIRED libnetconf2)  # check if the version is v1 latest or v2 latest
message(STATUS "${libnetconf2_INCLUDE_DIRS}")
message(STATUS "${libnetconf2_LINK_LIBRARIES}")

pkg_check_modules(libyang REQUIRED libyang)  # check if the version is v1 latest or v2 latest
message(STATUS "${libyang_INCLUDE_DIRS}")
message(STATUS "${libyang_LINK_LIBRARIES}")

pkg_check_modules(libxml-2.0 REQUIRED libxml-2.0)
message(STATUS "${libxml-2.0_INCLUDE_DIRS}")
message(STATUS "${libxml-2.0_LINK_LIBRARIES}")

add_executable(my_client main.c
                        init-mplane.c
                        connect-mplane.c
                        get-mplane.c
                        subscribe-mplane.c
                        config-mplane.c
                        disconnect-mplane.c
                        rpc-send-recv.c
                        xml/get-xml.c
                        yang/get-yang-models.c)

target_include_directories(my_client PRIVATE ${libnetconf2_INCLUDE_DIRS})
target_link_libraries(my_client PRIVATE ${libnetconf2_LINK_LIBRARIES})

target_include_directories(my_client PRIVATE ${libyang_INCLUDE_DIRS})
target_link_libraries(my_client PRIVATE ${libyang_LDFLAGS} ${libyang_LINK_LIBRARIES})

target_include_directories(my_client PRIVATE ${libxml-2.0_INCLUDE_DIRS})
target_link_libraries(my_client PRIVATE ${libxml-2.0_LINK_LIBRARIES})

target_compile_definitions(my_client PRIVATE ${MPLANE_VERSION})

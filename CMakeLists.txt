cmake_minimum_required(VERSION 3.5)

project(netem C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")

add_executable(netem main.c
	probe.c
	emulate.c
	timing.c
	hist.c
	utils.c
	ts.c
	tc.c
	dist.c
	dist-maketable.c
)

target_link_libraries(netem PUBLIC "-lrt -lnl-3 -lnl-route-3 -lm")
target_include_directories(netem PUBLIC "/usr/include/libnl3")

add_library(mark mark.c)

install(TARGETS netem mark 
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)

set(CPACK_PACKAGE_NAME "netem")
set(CPACK_PACKAGE_CONTACT "Steffen Vogel <post@steffenvogel.de>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "netem measures and emulates network delay distributions using the NetEm queuing discipline")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "iproute2")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://github.com/stv0g/netem")

set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
set(CPACK_RPM_PACKAGE_URL "http://github.com/stv0g/netem")
set(CPACK_RPM_PACKAGE_REQUIRES "iproute2")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_GENERATOR "DEB;RPM;TGZ")

include(CPack)


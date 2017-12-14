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
	tcp.c
	dist.c
	dist-maketable.c
)

target_link_libraries(netem PUBLIC "-lrt -lnl-3 -lnl-route-3 -lm")
target_include_directories(netem PUBLIC "/usr/include/libnl3")

add_library(mark mark.c)

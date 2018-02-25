cmake_minimum_required(VERSION 2.6)

project(test)

link_libraries(main pthread)

include_directories(include)

add_executable(main 
	src/fullyprobe.cpp
	src/scanner.cpp
	src/board.cpp
	src/main.cpp
	src/Hash.cpp
	src/set.cpp
	src/probsolver.cpp
	src/linesolve.cpp
	src/help.cpp
	src/options.cpp
	src/mirror.cpp
)

install(TARGETS main RUNTIME DESTINATION bin)

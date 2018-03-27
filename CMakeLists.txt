cmake_minimum_required(VERSION 2.8)

project(grey_proxy CXX)

find_package (Threads REQUIRED) 
# we will use static libs
set(Boost_USE_STATIC_LIBS ON)

# Boost::log required Boost version >= 1.54.0 
find_package(Boost 1.54.0 REQUIRED COMPONENTS log)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set(SRC_FILES
    main.cpp
	Server.cpp
	Server.h
	Connection.h
	Connection.cpp)

add_definitions(-D_WIN32_WINNT=0x0601)
add_executable(grey_proxy ${SRC_FILES})
target_link_libraries(grey_proxy ${Boost_LIBRARIES} Threads::Threads)

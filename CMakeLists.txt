cmake_minimum_required(VERSION 2.8)

project(grey_proxy CXX)
add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package (Threads REQUIRED) 
# we will use static libs
#set(Boost_USE_STATIC_LIBS OFF)
add_definitions(-DBOOST_LOG_DYN_LINK)
# Boost::log required Boost version >= 1.54.0 
find_package(Boost 1.54.0 REQUIRED COMPONENTS log)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set(SRC_FILES
    main.cpp
	Server.cpp
	Server.h
	Connection.h
	Connection.cpp
	HttpHeader.h
	HttpHeader.cpp
	HttpHeaderParser.h
	HttpHeaderParser.cpp
	HttpRequestHeader.h
	HttpRequestHeader.cpp
	HttpResponseHeader.h
	HttpResponseHeader.cpp)

if (MSVC)
	add_definitions(-D_WIN32_WINNT=0x0601)
	add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
	add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
	add_compile_options(/wd4834)
	add_definitions(-DBOOST_LOG_DYN_LINK)
endif(MSVC)
add_executable(grey_proxy ${SRC_FILES})
target_link_libraries(grey_proxy ${Boost_LIBRARIES} Threads::Threads)

cmake_minimum_required(VERSION 3.7)
project(cnc)

#set(CMAKE_LIBRARY_PATH "${LD_LIBRARY_PATH}")
set(CMAKE_CXX_COMPILER "/home/fw591/local/gcc-7.2.0/bin/g++")

set(CNC_GTK_VERSION "2")
set(CNC_USER_HOME   "/home/fw591")
set(CNC_SPACE_HOME  "${CNC_USER_HOME}/dev")
set(CNC_BUILD_HOME  "${CNC_SPACE_HOME}/cnc")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /home/fw591/local/lib/wx/include/gtk2-unicode-3.1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /home/fw591/local/include/wx-3.1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /home/fw591/local/include/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /home/fw591/glew-2.1.0/include")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/fw591/local/lib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/fw591/local/lib/usr/local/lib64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,/home/fw591/local/lib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--version-script,/home/fw591/wxWidgets-3.1.0/gtk-build/version-script -lz -ldl -lm  -lz -ldl -lm -flto")

include_directories("/home/fw591/boost_1_66_0")
include_directories("/home/fw591/local/include/wx-3.1")
include_directories("/home/fw591/freeglut-3.0.0/include")
include_directories("/home/fw591/dev/cnc/CncControlerGui")

link_directories("/home/fw591/local/lib")
#link_directories()
#link_directories()

include(CMakeListsMain.txt)

cmake_minimum_required(VERSION 3.8)
project(server VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
#可执行文件的路径
set(HOME /home/huaan/Desktop/Project/C/serverLearn/ReactorHttpServer/CPP)
#set(EXECUTABLE_OUTPUT_PATH ${HOME}/bin)
#指定输出的库目录
set(LIBRARY_OUTPUT_PATH ${HOME}/libs)
#依赖源文件
file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
#aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)
#set(SRC ./src/*.c)
#头文件
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/include/mysql)
#使用库文件
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)
# link_directories(/usr/lib/mysql)
# link_libraries(jsoncpp )
#制作库文件
#动态库
#add_library(server SHARED ${SRC})          
#静态库
add_library(server STATIC ${SRC})
#制作可执行程序
#add_executable(server ${SRC})
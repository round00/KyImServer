cmake_minimum_required(VERSION 3.14)
#cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -Wall -O0")

project ("KyImServer")

#增加头文件包含目录
include_directories(
	/usr/include/mysql/ 
)
#增加链接库目录
link_directories(
	/usr/lib64/mysql/
)

# 将源代码添加到此项目的可执行文件。
add_executable (KyImServer main.cpp ConfigFile.cpp util.cpp db/MysqlDb.cpp db/MysqlQueryResult.cpp)
target_link_libraries(KyImServer mysqlclient)


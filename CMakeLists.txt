cmake_minimum_required(VERSION 3.14)
#cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -Wall -O0")

project ("KyImServer")

#增加头文件包含目录
include_directories(
	/usr/include/mysql/
	/usr/local/include/
	db/
	base/
	net/
	thirdparty/
)
#增加链接库目录
link_directories(
	/usr/lib64/mysql/
	/usr/local/lib/
)

add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(db)
add_subdirectory(thirdparty)

add_subdirectory(FileServer)
add_subdirectory(ImServer)



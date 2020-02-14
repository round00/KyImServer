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
)
#增加链接库目录
link_directories(
	/usr/lib64/mysql/
	/usr/local/lib/
)

add_subdirectory(base)
add_subdirectory(db)
add_subdirectory(thirdparty)

include_directories(thirdparty/)
set(chatserver_src
		main.cpp
		ConfigFile.cpp
		UserManager.cpp

		EventLoop.cpp
		TcpServer.cpp
		ImServer.cpp
		Session.cpp
		ClientSession.cpp
		ProtocolStream.cpp
		Buffer.cpp
		FriendCroup.cpp
		)
# 将源代码添加到此项目的可执行文件。
add_executable (KyImServer ${chatserver_src})
target_link_libraries(KyImServer base db trdparty event)


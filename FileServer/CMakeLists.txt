set(FILESERVER_SRC
        main.cpp
        FileServer.cpp
        FileSession.cpp
        FileManager.cpp
        )
# 将源代码添加到此项目的可执行文件。
add_executable(FileServer ${FILESERVER_SRC})
target_link_libraries(FileServer base db trdparty net)


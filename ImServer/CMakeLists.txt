set(IMSERVER_SRC
        main.cpp
        EntityManager.cpp
        ImServer.cpp
        ClientSession.cpp
        FriendCroup.cpp
        )
# 将源代码添加到此项目的可执行文件。
add_executable (ImServer ${IMSERVER_SRC})
target_link_libraries(ImServer base db trdparty net)


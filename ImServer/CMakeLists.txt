set(IMSERVER_SRC
        main.cpp
        EntityManager.cpp
        ImServer.cpp
        ClientSession.cpp
        FriendCroup.cpp
        )
# 将源代码添加到此项目的可执行文件。
add_executable (KyImServer ${IMSERVER_SRC})
target_link_libraries(KyImServer base db trdparty net)


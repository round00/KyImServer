set(NET_SRC
        EventLoop.cpp
        ProtocolStream.cpp
        TcpServer.cpp
        TcpSession.cpp
        )

add_library(net ${NET_SRC})
target_link_libraries(net event)

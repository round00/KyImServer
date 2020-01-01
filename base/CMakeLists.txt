set(BASE_SRC
        Logger.cpp
        util.cpp
        Mutex.cpp
        MutexLock.cpp
        Condition.cpp
        Thread.cpp)

add_library(base ${BASE_SRC})
target_link_libraries(base pthread)
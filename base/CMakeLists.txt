set(BASE_SRC
        Logger.cpp
        util.cpp
        Mutex.cpp
        Condition.cpp
        Thread.cpp
        ConfigFile.cpp
        )

add_library(base ${BASE_SRC})
target_link_libraries(base pthread)

add_subdirectory(test)
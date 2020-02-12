set(db_src
        MysqlDb.cpp
        MysqlQueryResult.cpp
        Redis.cpp)

add_library(db ${db_src})
target_link_libraries(db mysqlclient hiredis)

add_subdirectory(test)
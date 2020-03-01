set(db_src
        Redis.cpp)

add_library(db ${db_src})
target_link_libraries(db hiredis)

add_subdirectory(test)
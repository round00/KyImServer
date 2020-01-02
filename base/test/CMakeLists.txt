

add_executable(TestThread TestThread.cpp)
target_link_libraries(TestThread base)

add_executable(TestMutex TestMutex.cpp)
target_link_libraries(TestMutex base)

add_executable(TestCondition TestCondition.cpp)
target_link_libraries(TestCondition base)
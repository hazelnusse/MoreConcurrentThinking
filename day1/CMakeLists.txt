add_library(foo foo.cpp foo.hpp)
target_include_directories(foo PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# TODO: wrap all this into a function/macro to avoid all the boilerplate
add_executable(foo.test foo.test.cpp)
target_link_libraries(foo.test foo boost.ut)
#target_link_libraries(foo.test boost.ut)
add_dependencies(foo.test project_boost.ut)
add_test(NAME foo.test COMMAND foo.test)

add_executable(main main.cpp)
add_executable(exercise1 exercise1.cpp)
target_link_libraries(exercise1 Threads::Threads)

add_executable(exercise2 exercise2.cpp)
target_link_libraries(exercise2 Threads::Threads)

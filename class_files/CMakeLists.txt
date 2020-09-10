add_library(actors INTERFACE)
target_include_directories(actors INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(actors INTERFACE Threads::Threads)

add_executable(pingpongactors pingpongactors.cpp)
target_link_libraries(pingpongactors actors)

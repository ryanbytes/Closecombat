cmake_minimum_required(VERSION 3.22.1)
project(closecombat LANGUAGES CXX)

add_library(
    closecombat
    SHARED
    native-lib.cpp
    Engine.cpp
)

target_compile_features(closecombat PRIVATE cxx_std_20)

find_library(log-lib log)

target_link_libraries(
    closecombat
    ${log-lib}
)

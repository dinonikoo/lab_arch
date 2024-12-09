cmake_minimum_required(VERSION 3.10)

project(inverse_lab)
find_package(OpenCV REQUIRED)
add_executable(inverse_lab main.cpp)

target_link_libraries(inverse_lab ${OpenCV_LIBS})

cmake_minimum_required(VERSION 3.10)  
project(LabaArch3)                  

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(OpenCV_DIR /usr/lib/x86_64-linux-gnu/cmake/opencv4/OpenCVConfig.cmake) #где находится open cv
find_package(OpenCV REQUIRED)

include_directories(include ${OpenCV_INCLUDE_DIRS})

set(SOURCES
    src/main.cpp
    src/consumer.cpp
    src/producer.cpp
    src/task_queue.cpp
)

add_executable(LabaArch3 ${SOURCES})

target_link_libraries(LabaArch3 ${OpenCV_LIBS})

cmake_minimum_required(VERSION 3.10)
project(camera_prog)

set(CMAKE_CXX_STANDARD 14)
find_package (Threads REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} "/usr/local/include/opencv4/")

add_executable(camera_prog main.cpp base64.cpp utils.cpp)

target_link_libraries(${PROJECT_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OpenCV_LIBS}
        )
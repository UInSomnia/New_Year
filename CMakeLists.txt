cmake_minimum_required(VERSION 3.12)
project(New_Year CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

file(GLOB HEADERS "src/*.h")
file(GLOB SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME}
	${HEADERS}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	${OpenCV_LIBS})

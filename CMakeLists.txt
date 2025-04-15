cmake_minimum_required(VERSION 3.10)
project(IMUProject)

set(CMAKE_CXX_STANDARD 11)

# Include paths can be added here if needed
include_directories(/usr/local/include)

# Link directories
link_directories(/usr/local/lib)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# First executable: read_imu (uses RTIMULib)
add_executable(read_imu read_imu.cpp)
add_executable(display_imu display_imu.cpp)
target_link_libraries(read_imu RTIMULib)
target_link_libraries(display_imu RTIMULib ${CURSES_LIBRARIES})

# Second executable: make_arff (no special libs)
add_executable(make_arff make_arff.cpp)

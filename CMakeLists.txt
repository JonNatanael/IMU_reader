cmake_minimum_required(VERSION 3.10)
project(IMUProject)

set(CMAKE_CXX_STANDARD 11)

# Include paths can be added here if needed
include_directories(/usr/local/include)

# Link directories
link_directories(/usr/local/lib)

# First executable: read_imu (uses RTIMULib)
add_executable(read_imu read_imu.cpp)
target_link_libraries(read_imu RTIMULib)

# Second executable: make_arff (no special libs)
add_executable(make_arff make_arff.cpp)

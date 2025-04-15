# IMU Project

This project builds two C++ programs:

- `read_imu`: Reads data from an IMU using RTIMULib
- `make_arff`: Converts data to ARFF format

## Requirements

- CMake >= 3.10
- g++
- RTIMULib installed and accessible at `/usr/local/lib` and `/usr/local/include`

## Build Instructions

1. Ensure RTIMULib is installed.
2. Clone/download this project.
3. Run the build script:

```bash
./build.sh

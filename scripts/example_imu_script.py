#!/usr/bin/python3
import time
import sys
from slimmed_sense_hat import SenseHat

# Create an instance of the SenseHat class with a custom I2C address
custom_i2c_address = 0x6a  # Replace with your desired I2C address
sense = SenseHat(imu_i2c_address=custom_i2c_address)

# Function to read and print IMU data
def read_imu_data():
    # Clear screen
    print("\033[H", end="")

    # Get orientation in degrees
    orientation = sense.get_orientation_degrees()
    print("Orientation (degrees):")
    print(f"Pitch: {orientation['pitch']:.2f}")
    print(f"Roll: {orientation['roll']:.2f}")
    print(f"Yaw: {orientation['yaw']:.2f}")
    print()

    # Get raw accelerometer data
    accel_raw = sense.get_accelerometer_raw()
    print("Accelerometer (Gs):")
    print(f"X: {accel_raw['x']:.4f}")
    print(f"Y: {accel_raw['y']:.4f}")
    print(f"Z: {accel_raw['z']:.4f}")
    print()

    # Get raw gyroscope data
    gyro_raw = sense.get_gyroscope_raw()
    print("Gyroscope (radians/s):")
    print(f"X: {gyro_raw['x']:.4f}")
    print(f"Y: {gyro_raw['y']:.4f}")
    print(f"Z: {gyro_raw['z']:.4f}")
    print()

    # Get raw magnetometer data
    compass_raw = sense.get_compass_raw()
    print("Magnetometer (uT):")
    print(f"X: {compass_raw['x']:.4f}")
    print(f"Y: {compass_raw['y']:.4f}")
    print(f"Z: {compass_raw['z']:.4f}")
    print()

if __name__ == "__main__":
    try:
        while True:
            read_imu_data()
            time.sleep(0.1)  # Adjust for readability if needed
    except KeyboardInterrupt:
        print("\nExiting.")
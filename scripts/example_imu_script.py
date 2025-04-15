#!/usr/bin/python3
import time
from slimmed_sense_hat import SenseHat

# Create an instance of the SenseHat class with a custom I2C address
custom_i2c_address = 0x6a  # Replace with your desired I2C address
sense = SenseHat(imu_i2c_address=custom_i2c_address)

# Function to read and print IMU data
def read_imu_data():
    # Get orientation in degrees
    orientation = sense.get_orientation_degrees()
    print("Orientation (degrees):")
    print(f"Pitch: {orientation['pitch']}")
    print(f"Roll: {orientation['roll']}")
    print(f"Yaw: {orientation['yaw']}")

    # Get raw accelerometer data
    accel_raw = sense.get_accelerometer_raw()
    print("Accelerometer (Gs):")
    print(f"X: {accel_raw['x']}")
    print(f"Y: {accel_raw['y']}")
    print(f"Z: {accel_raw['z']}")

    # Get raw gyroscope data
    gyro_raw = sense.get_gyroscope_raw()
    print("Gyroscope (radians/s):")
    print(f"X: {gyro_raw['x']}")
    print(f"Y: {gyro_raw['y']}")
    print(f"Z: {gyro_raw['z']}")

    # Get raw magnetometer data
    compass_raw = sense.get_compass_raw()
    print("Magnetometer (uT):")
    print(f"X: {compass_raw['x']}")
    print(f"Y: {compass_raw['y']}")
    print(f"Z: {compass_raw['z']}")

if __name__ == "__main__":
    while True:
        read_imu_data()
        time.sleep(1)  # Read data every second

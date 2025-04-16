import numpy as np
import matplotlib.pyplot as plt

def read_and_display_imu_data(filepath):
    with open(filepath, 'r') as file:
        # Read header
        header = file.readline().strip().split('\t')
        print("Headers:", header)

        # Read data rows
        data = []
        for line in file:
            parts = line.strip().split('\t')
            if len(parts) != len(header):
                continue  # Skip malformed lines

            # First column: int (measurement ID)
            # Second column: timestamp string
            # Third column: int (event)
            # Remaining columns: floats
            row = {
                'Measurement': int(parts[0]),
                'Timestamp': parts[1],
                'Event': int(parts[2]),
                'GyroX': float(parts[3]),
                'GyroY': float(parts[4]),
                'GyroZ': float(parts[5]),
                'AccelX': float(parts[6]),
                'AccelY': float(parts[7]),
                'AccelZ': float(parts[8]),
                'MagX': float(parts[9]),
                'MagY': float(parts[10]),
                'MagZ': float(parts[11]),
            }
            data.append(row)

    # Display first few rows
    print("\nData:")
    for row in data:
        print(row)

    return header, data

def plot_imu_data(data):
    # Extract measurement numbers as x-axis
    x = np.array([row['Measurement'] for row in data])

    # Extract IMU data
    gyro = np.array([[row['GyroX'], row['GyroY'], row['GyroZ']] for row in data])
    accel = np.array([[row['AccelX'], row['AccelY'], row['AccelZ']] for row in data])
    mag = np.array([[row['MagX'], row['MagY'], row['MagZ']] for row in data])

    # Create subplots
    fig, axs = plt.subplots(3, 1, figsize=(10, 8), sharex=True)

    # Gyroscope
    axs[0].plot(x, gyro[:, 0], label='GyroX')
    axs[0].plot(x, gyro[:, 1], label='GyroY')
    axs[0].plot(x, gyro[:, 2], label='GyroZ')
    axs[0].set_ylabel('Gyro (rad/s)')
    axs[0].legend()
    axs[0].set_title('Gyroscope Readings')

    # Accelerometer
    axs[1].plot(x, accel[:, 0], label='AccelX')
    axs[1].plot(x, accel[:, 1], label='AccelY')
    axs[1].plot(x, accel[:, 2], label='AccelZ')
    axs[1].set_ylabel('Accel (G)')
    axs[1].legend()
    axs[1].set_title('Accelerometer Readings')

    # Magnetometer
    axs[2].plot(x, mag[:, 0], label='MagX')
    axs[2].plot(x, mag[:, 1], label='MagY')
    axs[2].plot(x, mag[:, 2], label='MagZ')
    axs[2].set_ylabel('Mag (uT)')
    axs[2].legend()
    axs[2].set_title('Magnetometer Readings')
    axs[2].set_xlabel('Measurement #')

    plt.tight_layout()
    plt.show()

def main():

    filepath = '/home/pi/data/0003.txt'
    header, data = read_and_display_imu_data(filepath)
    plot_imu_data(data)


if __name__=='__main__':
    main()

# Example usage:
# header, data = read_and_display_imu_data("your_data.txt")
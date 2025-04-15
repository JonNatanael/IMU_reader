import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from slimmed_sense_hat import SenseHat

sense = SenseHat(imu_i2c_address=0x6a)

# Buffer size
N = 100
# N = 50
x_vals = deque(maxlen=N)
y_vals = deque(maxlen=N)
z_vals = deque(maxlen=N)

fig, ax = plt.subplots()
line_x, = ax.plot([], [], label='X')
line_y, = ax.plot([], [], label='Y')
line_z, = ax.plot([], [], label='Z')
ax.set_ylim(-2, 2)
ax.set_xlim(0, N)
ax.legend()
ax.set_title('Live Accelerometer Data')

def update(frame):
    accel = sense.get_accelerometer_raw()
    print(accel)

    x_vals.append(accel['x'])
    y_vals.append(accel['y'])
    z_vals.append(accel['z'])

    line_x.set_data(range(len(x_vals)), x_vals)
    line_y.set_data(range(len(y_vals)), y_vals)
    line_z.set_data(range(len(z_vals)), z_vals)

    return line_x, line_y, line_z

ani = animation.FuncAnimation(fig, update, interval=10, blit=True)
plt.show()

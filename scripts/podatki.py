from sense_hat import SenseHat
import datetime

sense = SenseHat()

ax_, ay_, az_ = 0, 0, 0
last_vz = 0
obrat = 1
n = 0
t0 = datetime.datetime.now()
deltat_s = 0
file_obdelava = open("obdelava_podatkov.txt","w")
file_zasuk = open("podatki_zasuka.txt", "w")


#inicializacija 10 sekund
while deltat_s<10:
    deltat = datetime.datetime.now() - t0
    deltat_s = deltat.total_seconds()
    accel = sense.get_accelerometer_raw()
    ax = accel['x']
    ay = accel['y']
    az = accel['z']
    ax_ = ax_ + ax
    ay_ = ay_ + ay
    az_ = az_ + az
    n = n + 1
    
ax_p = ax_/n
ay_p = ay_/n
az_p = az_/n



while True:
    accel = sense.get_accelerometer_raw()
    time1 = datetime.datetime.now()
    time2 = time1.time()
    acc_x = accel['x']
    acc_y = accel['y']
    acc_z = accel['z']  

    #popravek šuma
    axP = acc_x - ax_p
    ayP = acc_y - ay_p
    azP = acc_z - az_p

        
    o = sense.get_orientation()
    time3 = datetime.datetime.now()
    time4 = time3.time()
    pitch = o["pitch"]
    roll = o["roll"]
    yaw = o["yaw"]

    print("aX = ","{:.2f}".format(axP), "\t aY = " "{:.2f}".format(ayP),"\t aZ = ","{:.2f}".format(azP),"\t roll = ", "{:.2f}".format(pitch),"\t pitch = ","{:.2f}".format(roll), "\t yaw = ","{:.2f}".format(yaw))

    file_obdelava.write("{}, {}, {}, {}\n".format(axP,ayP,azP,time2))
    file_zasuk.write("{}, {}, {}, {}\n".format(pitch, roll, yaw, time4))
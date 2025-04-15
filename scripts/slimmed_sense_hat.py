#!/usr/bin/python
import logging
import struct
import os
import sys
import math
import time
import numpy as np
import shutil
import glob
import RTIMU  # custom version
import pwd
import array
import fcntl
from copy import deepcopy

class SenseHat(object):

    SETTINGS_HOME_PATH = '.config/sense_hat'

    def __init__(
            self,
            imu_settings_file='RTIMULib',
            imu_i2c_address=0x6A  # Default I2C address for the IMU
        ):

        if not glob.glob('/dev/i2c*'):
            raise OSError('Cannot access I2C. Please ensure I2C is enabled in raspi-config')

        # Store the I2C address
        self.imu_i2c_address = imu_i2c_address

        # Load IMU settings and calibration data
        self._imu_settings = self._get_settings_file(imu_settings_file)
        self._imu = RTIMU.RTIMU(self._imu_settings)
        self._imu_init = False  # Will be initialised as and when needed
        self._last_orientation = {'pitch': 0, 'roll': 0, 'yaw': 0}
        raw = {'x': 0, 'y': 0, 'z': 0}
        self._last_compass_raw = deepcopy(raw)
        self._last_gyro_raw = deepcopy(raw)
        self._last_accel_raw = deepcopy(raw)
        self._compass_enabled = False
        self._gyro_enabled = False
        self._accel_enabled = False

        # Set the I2C address in the settings file
        self._set_imu_i2c_address(self.imu_i2c_address)

    def _get_settings_file(self, imu_settings_file):
        """
        Internal. Logic to check for a system wide RTIMU ini file. This is
        copied to the home folder if one is not already found there.
        """

        ini_file = '%s.ini' % imu_settings_file

        home_dir = pwd.getpwuid(os.getuid())[5]
        home_path = os.path.join(home_dir, self.SETTINGS_HOME_PATH)
        if not os.path.exists(home_path):
            os.makedirs(home_path)

        home_file = os.path.join(home_path, ini_file)
        home_exists = os.path.isfile(home_file)
        system_file = os.path.join('/etc', ini_file)
        system_exists = os.path.isfile(system_file)

        if system_exists and not home_exists:
            shutil.copyfile(system_file, home_file)

        return RTIMU.Settings(os.path.join(home_path, imu_settings_file))  # RTIMU will add .ini internally

    def _set_imu_i2c_address(self, i2c_address):
        """
        Internal. Sets the I2C address in the IMU settings file.
        """
        settings_file_path = os.path.join(
            pwd.getpwuid(os.getuid())[5], 
            self.SETTINGS_HOME_PATH, 
            'RTIMULib.ini'
        )

        with open(settings_file_path, 'r') as file:
            lines = file.readlines()

        with open(settings_file_path, 'w') as file:
            for line in lines:
                if 'I2CSlaveAddress' in line:
                    file.write(f'I2CSlaveAddress={i2c_address}\n')
                else:
                    file.write(line)

    ####
    # IMU Sensor
    ####

    def _init_imu(self):
        """
        Internal. Initialises the IMU sensor via RTIMU
        """

        if not self._imu_init:
            self._imu_init = self._imu.IMUInit()
            if self._imu_init:
                self._imu_poll_interval = self._imu.IMUGetPollInterval() * 0.001
                # Enable everything on IMU
                self.set_imu_config(True, True, True)
            else:
                raise OSError('IMU Init Failed')

    def set_imu_config(self, compass_enabled, gyro_enabled, accel_enabled):
        """
        Enables and disables the gyroscope, accelerometer and/or magnetometer
        input to the orientation functions
        """

        # If the consuming code always calls this just before reading the IMU
        # the IMU consistently fails to read. So prevent unnecessary calls to
        # IMU config functions using state variables

        self._init_imu()  # Ensure imu is initialised

        if (not isinstance(compass_enabled, bool)
        or not isinstance(gyro_enabled, bool)
        or not isinstance(accel_enabled, bool)):
            raise TypeError('All set_imu_config parameters must be of boolean type')

        if self._compass_enabled != compass_enabled:
            self._compass_enabled = compass_enabled
            self._imu.setCompassEnable(self._compass_enabled)

        if self._gyro_enabled != gyro_enabled:
            self._gyro_enabled = gyro_enabled
            self._imu.setGyroEnable(self._gyro_enabled)

        if self._accel_enabled != accel_enabled:
            self._accel_enabled = accel_enabled
            self._imu.setAccelEnable(self._accel_enabled)

    def _read_imu(self):
        """
        Internal. Tries to read the IMU sensor three times before giving up
        """

        self._init_imu()  # Ensure imu is initialised

        attempts = 0
        success = False

        while not success and attempts < 3:
            success = self._imu.IMURead()
            attempts += 1
            time.sleep(self._imu_poll_interval)

        return success

    def _get_raw_data(self, is_valid_key, data_key):
        """
        Internal. Returns the specified raw data from the IMU when valid
        """

        result = None

        if self._read_imu():
            data = self._imu.getIMUData()
            if data[is_valid_key]:
                raw = data[data_key]
                result = {
                    'x': raw[0],
                    'y': raw[1],
                    'z': raw[2]
                }

        return result

    def get_orientation_radians(self):
        """
        Returns a dictionary object to represent the current orientation in
        radians using the aircraft principal axes of pitch, roll and yaw
        """

        raw = self._get_raw_data('fusionPoseValid', 'fusionPose')

        if raw is not None:
            raw['roll'] = raw.pop('x')
            raw['pitch'] = raw.pop('y')
            raw['yaw'] = raw.pop('z')
            self._last_orientation = raw

        return deepcopy(self._last_orientation)

    @property
    def orientation_radians(self):
        return self.get_orientation_radians()

    def get_orientation_degrees(self):
        """
        Returns a dictionary object to represent the current orientation
        in degrees, 0 to 360, using the aircraft principal axes of
        pitch, roll and yaw
        """

        orientation = self.get_orientation_radians()
        for key, val in orientation.items():
            deg = math.degrees(val)  # Result is -180 to +180
            orientation[key] = deg + 360 if deg < 0 else deg
        return orientation

    def get_orientation(self):
        return self.get_orientation_degrees()

    @property
    def orientation(self):
        return self.get_orientation_degrees()

    def get_compass(self):
        """
        Gets the direction of North from the magnetometer in degrees
        """

        self.set_imu_config(True, False, False)
        orientation = self.get_orientation_degrees()
        if type(orientation) is dict and 'yaw' in orientation.keys():
            return orientation['yaw']
        else:
            return None

    @property
    def compass(self):
        return self.get_compass()

    def get_compass_raw(self):
        """
        Magnetometer x y z raw data in uT (micro teslas)
        """

        raw = self._get_raw_data('compassValid', 'compass')

        if raw is not None:
            self._last_compass_raw = raw

        return deepcopy(self._last_compass_raw)

    @property
    def compass_raw(self):
        return self.get_compass_raw()

    def get_gyroscope(self):
        """
        Gets the orientation in degrees from the gyroscope only
        """

        self.set_imu_config(False, True, False)
        return self.get_orientation_degrees()

    @property
    def gyro(self):
        return self.get_gyroscope()

    @property
    def gyroscope(self):
        return self.get_gyroscope()

    def get_gyroscope_raw(self):
        """
        Gyroscope x y z raw data in radians per second
        """

        raw = self._get_raw_data('gyroValid', 'gyro')

        if raw is not None:
            self._last_gyro_raw = raw

        return deepcopy(self._last_gyro_raw)

    @property
    def gyro_raw(self):
        return self.get_gyroscope_raw()

    @property
    def gyroscope_raw(self):
        return self.get_gyroscope_raw()

    def get_accelerometer(self):
        """
        Gets the orientation in degrees from the accelerometer only
        """

        self.set_imu_config(False, False, True)
        return self.get_orientation_degrees()

    @property
    def accel(self):
        return self.get_accelerometer()

    @property
    def accelerometer(self):
        return self.get_accelerometer()

    def get_accelerometer_raw(self):
        """
        Accelerometer x y z raw data in Gs
        """

        raw = self._get_raw_data('accelValid', 'accel')

        if raw is not None:
            self._last_accel_raw = raw

        return deepcopy(self._last_accel_raw)

    @property
    def accel_raw(self):
        return self.get_accelerometer_raw()

    @property
    def accelerometer_raw(self):
        return self.get_accelerometer_raw()

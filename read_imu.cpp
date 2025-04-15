#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <RTIMULib.h>
#include <time.h>
#include <getopt.h>
#include <vector>
#include <algorithm>
#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>

#define IMU_SETTINGS_FILE "RTIMULib"
#define STEP_INCREMENT 0.05  // Step increment for threshold and delta adjustments
#define DATA_FOLDER "/home/pi/data/"

// Function to clear the terminal screen
void clear_screen() {
    printf("\033[2J"); // ANSI escape code to clear the screen
}

// Function to move the cursor to the specified position
void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col); // ANSI escape code to move the cursor
}

// Function to calculate the elapsed time in milliseconds
long get_elapsed_milliseconds(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}

// Function to print the IMU data
void print_imu_data(RTIMU_DATA imuData) {
    move_cursor(1, 1);
    printf("Gyroscope:     (%.2f, %.2f, %.2f) rad/s          \n",
           imuData.gyro.x(), imuData.gyro.y(), imuData.gyro.z());
    move_cursor(2, 1);
    printf("Accelerometer: (%.2f, %.2f, %.2f) G          \n",
           imuData.accel.x(), imuData.accel.y(), imuData.accel.z());
    move_cursor(3, 1);
    printf("Magnetometer:  (%.2f, %.2f, %.2f) uT          \n",
           imuData.compass.x(), imuData.compass.y(), imuData.compass.z());
    fflush(stdout); // Ensure that the output is written to the terminal
}

// Function to print the command menu and current threshold/delta values
void print_commands(float threshold, float delta) {
    move_cursor(7, 1);
    printf("Threshold = %.2f          \n", threshold);
    move_cursor(8, 1);
    printf("Delta = %.2f          \n", delta);
    move_cursor(9, 1);
    printf("Commands:          \n");
    move_cursor(10, 1);
    printf("\tt: increase threshold          \n");
    move_cursor(11, 1);
    printf("\tr: decrease threshold          \n");
    move_cursor(12, 1);
    printf("\tf: increase delta          \n");
    move_cursor(13, 1);
    printf("\td: decrease delta          \n");
    move_cursor(14, 1);
    printf("\t[space]: start or end acquisition          \n");
    move_cursor(15, 1);
    printf("\tp: make .arff file from last 2x data          \n");
    move_cursor(16, 1);
    printf("\tP: delete all recorded data          \n");
    move_cursor(17, 1);
    printf("\tx: exit          \n");
    move_cursor(18, 1);
    printf("\tX: shutdown and exit          \n");
    fflush(stdout);
}

// Simple step detection based on the specified axis acceleration
int detect_step(std::vector<float>& accel_data, float threshold, float step_delta) {
    static int step_state = 0;

    if (accel_data.size() < 3) {
        return 0;  // Not enough data to detect a step
    }

    // Calculate the moving average of the last 3 values
    float avg = (accel_data[accel_data.size() - 1] + accel_data[accel_data.size() - 2] + accel_data[accel_data.size() - 3]) / 3.0;

    // Detecting a positive peak (potential step)
    if (step_state == 0 && avg > threshold + step_delta) {
        step_state = 1;
        return 1;  // Step detected
    } 
    // Detecting a negative peak (confirming the step)
    else if (step_state == 1 && avg < threshold - step_delta) {
        step_state = 0;
    }

    return 0;  // No step detected
}

// Function to set the terminal to non-blocking mode
void set_nonblocking_input() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

// Function to reset the terminal to blocking mode
void reset_terminal_mode() {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    oldt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, ~O_NONBLOCK);
}

// Function to format the elapsed time into HH:MM:SS.msec
void format_time(long elapsed_milliseconds, char* buffer) {
    int hours = elapsed_milliseconds / 3600000;
    int minutes = (elapsed_milliseconds % 3600000) / 60000;
    int seconds = (elapsed_milliseconds % 60000) / 1000;
    int milliseconds = elapsed_milliseconds % 1000;
    sprintf(buffer, "Recording: %02d:%02d:%02d.%04d          ", hours, minutes, seconds, milliseconds);
}

// Function to get the next available filename in the specified directory
std::string get_next_filename(const char* directory) {
    int file_index = 1;
    char filename[256];
    struct stat buffer;

    while (file_index <= 9999) {
        snprintf(filename, sizeof(filename), "%s%04d.txt", directory, file_index);
        if (stat(filename, &buffer) != 0) { // File does not exist
            return std::string(filename);
        }
        file_index++;
    }

    // If all filenames are taken (highly unlikely)
    return std::string();
}

// Function to initialize the IMU with default settings
RTIMU* initializeIMU(RTIMUSettings *settings) {
    RTIMU *imu = RTIMU::createIMU(settings);

    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL)) {
        printf("No IMU found\n");
        exit(1);
    }

    imu->IMUInit();
    imu->setSlerpPower(0.02);
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);

    return imu;
}

// Function to delete all .txt files in the data folder
void delete_all_files(const char* directory) {
    DIR* dir = opendir(directory);
    struct dirent* entry;
    char filepath[256];

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // If it's a regular file
            snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
            if (strstr(entry->d_name, ".txt") != NULL) { // Check if it has .txt extension
                if (remove(filepath) != 0) {
                    perror("remove");
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char* argv[]) {
    // Command-line options
    char axis = 'z';
    float threshold = 1.0;
    float step_delta = 0.3;

    int opt;
    while ((opt = getopt(argc, argv, "a:t:d:")) != -1) {
        switch (opt) {
            case 'a':
                axis = optarg[0];
                break;
            case 't':
                threshold = atof(optarg);
                break;
            case 'd':
                step_delta = atof(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-a axis] [-t threshold] [-d step_delta]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Initialize the settings file
    RTIMUSettings *settings = new RTIMUSettings(IMU_SETTINGS_FILE);
    
    // Initialize the IMU
    RTIMU *imu = initializeIMU(settings);

    // Set terminal to non-blocking input
    set_nonblocking_input();

    // Ensure terminal mode is reset when the program exits
    atexit(reset_terminal_mode);

    // Benchmark without printing
    int no_print_count = 0;
    struct timespec benchmark_start_time, benchmark_end_time;
    clock_gettime(CLOCK_MONOTONIC, &benchmark_start_time);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &benchmark_end_time);
        if (get_elapsed_milliseconds(benchmark_start_time, benchmark_end_time) >= 1000) {
            break;
        }
        if (imu->IMURead()) {
            no_print_count++;
        }
    }

    // Clear the screen before starting the loop
    clear_screen();

    // Variables for benchmarking with printing
    int with_print_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &benchmark_start_time);

    // Variables for step detection
    int step_count = 0;
    std::vector<float> accel_data;  // Buffer to store the last few accelerometer readings

    // Variables for command handling
    bool acquisition_active = false;
    long elapsed_milliseconds = 0;
    struct timespec recording_start_time, recording_end_time;
    char formatted_time[30];

    // Variables for recording
    int measurement_counter = 0;
    FILE* data_file = NULL;
    std::string current_filename;

    // Create data directory if it does not exist
    mkdir(DATA_FOLDER, 0777);

    // Print IMU data continuously
    while (1) {
        if (imu->IMURead()) {
            RTIMU_DATA imuData = imu->getIMUData();
            print_imu_data(imuData);
            with_print_count++;

            // Step detection based on the specified axis
            float accel_value;
            switch (axis) {
                case 'x':
                    accel_value = imuData.accel.x();
                    break;
                case 'y':
                    accel_value = imuData.accel.y();
                    break;
                case 'z':
                default:
                    accel_value = imuData.accel.z();
                    break;
            }

            // Add the current acceleration to the buffer
            accel_data.push_back(accel_value);
            if (accel_data.size() > 3) {
                accel_data.erase(accel_data.begin());  // Keep only the last 3 values
            }

            int step_detected = detect_step(accel_data, threshold, step_delta);

            if (step_detected) {
                step_count++;
                move_cursor(4, 1);
                printf("Step %d\a          \n", step_count); // '\a' generates a beep sound
            }

            clock_gettime(CLOCK_MONOTONIC, &benchmark_end_time);
            if (get_elapsed_milliseconds(benchmark_start_time, benchmark_end_time) >= 1000) {
                move_cursor(5, 1);
                printf("Readings per second (without printing): %d          \n", no_print_count);
                move_cursor(6, 1);
                printf("Readings per second (with printing):    %d          \n", with_print_count);

                // Reset the count and timer for the next second
                with_print_count = 0;
                clock_gettime(CLOCK_MONOTONIC, &benchmark_start_time);
            }

            // Print the command menu and current threshold/delta values
            print_commands(threshold, step_delta);

            // Update and print the recording timestamp if acquisition is active
            if (acquisition_active) {
                clock_gettime(CLOCK_MONOTONIC, &recording_end_time);
                elapsed_milliseconds = get_elapsed_milliseconds(recording_start_time, recording_end_time);
                format_time(elapsed_milliseconds, formatted_time);
                move_cursor(19, 1);
                printf("%s", formatted_time);

                // Write data to file
                if (data_file) {
                    fprintf(data_file, "%d\t%02d:%02d:%02d.%04ld\t%d\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\t%.9f\n",
                            measurement_counter++, elapsed_milliseconds / 3600000, (elapsed_milliseconds % 3600000) / 60000,
                            (elapsed_milliseconds % 60000) / 1000, elapsed_milliseconds % 1000, step_detected,
                            imuData.gyro.x(), imuData.gyro.y(), imuData.gyro.z(),
                            imuData.accel.x(), imuData.accel.y(), imuData.accel.z(),
                            imuData.compass.x(), imuData.compass.y(), imuData.compass.z());
                }
            } else {
                move_cursor(19, 1);
                printf("                              \n");  // Clear the recording line
            }
        }

        // Handle keyboard input
        char ch = getchar();
        if (ch != EOF) {
            switch (ch) {
                case 't':
                    threshold += STEP_INCREMENT;
                    break;
                case 'r':
                    threshold -= STEP_INCREMENT;
                    break;
                case 'f':
                    step_delta += STEP_INCREMENT;
                    break;
                case 'd':
                    step_delta -= STEP_INCREMENT;
                    break;
                case ' ':
                    if (acquisition_active) {
                        acquisition_active = false;
                        if (data_file) {
                            fclose(data_file);
                            data_file = NULL;
                        }
                        move_cursor(18, 1);
                        printf("Acquisition ended, file saved as %s          \n", current_filename.c_str());
                    } else {
                        acquisition_active = true;
                        clock_gettime(CLOCK_MONOTONIC, &recording_start_time);
                        measurement_counter = 0;

                        // Open new data file
                        current_filename = get_next_filename(DATA_FOLDER);
                        data_file = fopen(current_filename.c_str(), "w");
                        if (data_file) {
                            fprintf(data_file, "Measurement\tTimestamp\tEvent\tGyroX\tGyroY\tGyroZ\tAccelX\tAccelY\tAccelZ\tMagX\tMagY\tMagZ\n");
                        }

                        move_cursor(18, 1);
                        printf("Acquisition started, writing to %s          \n", current_filename.c_str());
                    }
                    break;
                case 'p':
                    if (!acquisition_active) {
                        system("cd /home/pi/data && ./make_arff");
                    }
                    break;
                case 'P':
                    if (!acquisition_active) {
                        delete_all_files(DATA_FOLDER);
                        move_cursor(20, 1);
                        printf("All recorded data deleted.          \n");
                    }
                    break;
                case 'x':
                    reset_terminal_mode();
                    exit(0);
                case 'X':
                    reset_terminal_mode();
                    system("sudo poweroff");
                    exit(0);
            }
        }

        usleep(imu->IMUGetPollInterval() * 1000); // Sleep for the polling interval
    }

    // Clean up
    delete imu;
    delete settings;

    return 0;
}

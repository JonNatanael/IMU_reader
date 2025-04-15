#include <ncurses.h>
#include <RTIMULib.h>
#include <unistd.h>     // for usleep
#include <vector>
#include <cmath>
#include <string>

// Number of samples to keep in the rolling graph
static const int GRAPH_WIDTH = 160;
// Height of the graph region
static const int GRAPH_HEIGHT = 40;

// We expect accelerometer data to be in range ~[-2, 2] G for typical usage
static const float MIN_VAL = -2.0f;
static const float MAX_VAL =  2.0f;

// A small helper to map [minVal, maxVal] -> [0, graphHeight-1]
int scaleToGraph(float value, float minVal, float maxVal, int height)
{
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;

    // Normalize (0..1)
    float ratio = (value - minVal) / (maxVal - minVal);
    // Map to [0..height-1]
    int scaled = (int)(ratio * (height - 1));
    return scaled;
}

int main()
{
    // 1. Initialize IMU
    RTIMUSettings *settings = new RTIMUSettings("RTIMULib");
    RTIMU *imu = RTIMU::createIMU(settings);
    if (!imu || (imu->IMUType() == RTIMU_TYPE_NULL)) {
        printf("No IMU found\n");
        return 1;
    }
    imu->IMUInit();
    imu->setSlerpPower(0.02);
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);

    // 2. Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); // non-blocking getch()

    // Optional: enable color if terminal supports it
    if (has_colors()) {
        start_color();
        // Initialize some color pairs: 1=Red, 2=Green, 3=Blue
        init_pair(1, COLOR_RED,   COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_BLUE,  COLOR_BLACK);
    }

    // 3. Prepare ring buffers for X/Y/Z
    std::vector<float> bufX(GRAPH_WIDTH, 0.0f);
    std::vector<float> bufY(GRAPH_WIDTH, 0.0f);
    std::vector<float> bufZ(GRAPH_WIDTH, 0.0f);

    int index = 0; // ring buffer index
    while (true)
    {
        // -- Step A: read new accelerometer data --
        if (imu->IMURead()) {
            RTIMU_DATA data = imu->getIMUData();
            float ax = data.accel.x();
            float ay = data.accel.y();
            float az = data.accel.z();

            // Store into ring buffers
            bufX[index] = ax;
            bufY[index] = ay;
            bufZ[index] = az;

            // Advance index (wrap around)
            index = (index + 1) % GRAPH_WIDTH;
        }

        // -- Step B: Clear the screen
        erase();  // or clear()

        // Print a small title / legend above the plot
        mvprintw(0, 0, "IMU Accelerometer Live Plot (X=red, Y=green, Z=blue)");
        mvprintw(1, 0, "[Press Q to quit] Range: [%.1f .. %.1f] G", MIN_VAL, MAX_VAL);

        // -- Step C: Draw the ASCII graph starting around row 2 downward
        // We'll treat row=2 as the top of the plot and row=(2 + GRAPH_HEIGHT -1) as the bottom
        int plotTop = 2;
        int plotBottom = plotTop + GRAPH_HEIGHT - 1;

        // We'll treat each column in ring buffer as 1 step in the X direction
        // The ring buffer index is the "end" of the wave. We'll draw from index+1..index in a loop
        // going left to right on screen.
        for (int i = 0; i < GRAPH_WIDTH; i++)
        {
            // The ring-buffer position in the past. We'll invert i so that
            // the newest sample is at the far right, scrolling leftwards
            int bufPos = (index + i) % GRAPH_WIDTH; 
            int screenCol = i; // left side =0, right side=GRAPH_WIDTH-1

            // Scale X
            int scaledX = scaleToGraph(bufX[bufPos], MIN_VAL, MAX_VAL, GRAPH_HEIGHT);
            // The row on the terminal (note we invert row so 0 is top)
            int rowX = plotBottom - scaledX;

            // Scale Y
            int scaledY = scaleToGraph(bufY[bufPos], MIN_VAL, MAX_VAL, GRAPH_HEIGHT);
            int rowY = plotBottom - scaledY;

            // Scale Z
            int scaledZ = scaleToGraph(bufZ[bufPos], MIN_VAL, MAX_VAL, GRAPH_HEIGHT);
            int rowZ = plotBottom - scaledZ;

            // Plot them, if within the window
            // Use color pairs if available
            if (has_colors()) {
                attron(COLOR_PAIR(1));
                mvaddch(rowX, screenCol, 'x');
                attroff(COLOR_PAIR(1));

                attron(COLOR_PAIR(2));
                mvaddch(rowY, screenCol, 'y');
                attroff(COLOR_PAIR(2));

                attron(COLOR_PAIR(3));
                mvaddch(rowZ, screenCol, 'z');
                attroff(COLOR_PAIR(3));
            } else {
                // no color: just label them differently
                mvaddch(rowX, screenCol, 'X');
                mvaddch(rowY, screenCol, 'Y');
                mvaddch(rowZ, screenCol, 'Z');
            }
        }

        // -- Step D: refresh so changes appear
        refresh();

        // -- Step E: check keypress
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break; // exit loop
        }

        // Sleep for poll interval or a fixed time
        // The poll interval might be small, so we clamp it a bit
        usleep(imu->IMUGetPollInterval() * 50); // IMU-based or you can do a fixed ~50ms
    }

    // 4. End ncurses
    endwin();

    // Cleanup IMU
    delete imu;
    delete settings;

    return 0;
}

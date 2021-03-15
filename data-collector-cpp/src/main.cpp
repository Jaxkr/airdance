#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>

#include "JoyShockLibrary.h"
#include "SDL2/SDL.h"
#include "hidapi.h"

using std::cin;
using std::cout;
using std::endl;
using std::mutex;
using std::string;

#define LEFT_JOYCON 1
#define RIGHT_JOYCON 2

void connectDevices() { int numConnected = JslConnectDevices(); };

void cleanup() { JslDisconnectAndDisposeAll(); }

// USE THIS TO DETERMINE INDEX TO ANALYZE IN THE PYTHON SCRIPT.
struct imu_frame_t {
    // local acceleration without gravity
    float accelX;
    float accelY;
    float accelZ;
    // gravity vector
    float gravX;
    float gravY;
    float gravZ;
};

uint64_t lastStomp =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
unsigned int stompCounter = 0;

// holds the last n samples of IMU data from each foot;
#define BUFFER_SIZE 30
imu_frame_t left_imu_buffer[BUFFER_SIZE];
unsigned int left_imu_index = 0;
mutex m_left_imu_buffer;
imu_frame_t right_imu_buffer[BUFFER_SIZE];
unsigned int right_imu_index = 0;
mutex m_right_imu_buffer;

void addFrameToIMUBuffer(IMU_STATE imuState, MOTION_STATE motion, unsigned int foot_id) {
    imu_frame_t new_frame;
    new_frame.accelX = motion.accelX;
    new_frame.accelY = motion.accelY;
    new_frame.accelZ = motion.accelZ;
    new_frame.gravX = motion.gravX;
    new_frame.gravY = motion.gravY;
    new_frame.gravZ = motion.gravZ;

    mutex *m_imu_buffer;
    unsigned int *imu_index;
    imu_frame_t(*imu_buffer)[BUFFER_SIZE];
    if (foot_id == LEFT_JOYCON) {
        m_imu_buffer = &m_left_imu_buffer;
        imu_index = &left_imu_index;
        imu_buffer = &left_imu_buffer;
    } else if (foot_id == RIGHT_JOYCON) {
        m_imu_buffer = &m_right_imu_buffer;
        imu_index = &right_imu_index;
        imu_buffer = &right_imu_buffer;
    }

    (*m_imu_buffer).lock();
    (*imu_buffer)[*imu_index] = new_frame;
    *imu_index += 1;
    // the index is too big, wrap around.
    if (*imu_index == BUFFER_SIZE) {
        *imu_index = 0;
    }
    (*m_imu_buffer).unlock();
}

/*
We will be annotating the following events and exporting 200ms worth of sensor data for each.
LEFT_PRESS - the left pad is presssed
LEFT_RELEASE - the right pad is released
RIGHT_PRESS - ...
RIGHT_RELEASE = ...
and so forth...

This data will be coordinated using the actual dance mat data. This means the data will include unintentional stomps.
TODO: Consider maybe removing steps in close succession?

The format is the direction int and then each value of imu_frame_t in order.
This will return 30 imu_frames from the left and 30 from the right.

*/

std::ofstream ofile;
string filename = "../data/" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "-session.txt";

void getBuffer(int direction) {
    unsigned int *imu_index;
    imu_frame_t(*imu_buffer)[BUFFER_SIZE];

    string output = "";
    output += std::to_string(direction) + " ";

    // run once for each foot
    for (int j = 0; j < 2; j++) {
        if (j == 0) {
            imu_index = &left_imu_index;
            imu_buffer = &left_imu_buffer;
        } else if (j == 1) {
            imu_index = &right_imu_index;
            imu_buffer = &right_imu_buffer;
        }
        // since the buffer is designed as a last-in-first-out fixed-length queue, we start at the beginning of the
        // buffer wrapping around 0
        int i = abs((int)(*imu_index) - (BUFFER_SIZE - 1));
        int counter = 0;
        while (counter < BUFFER_SIZE) {
            output += std::to_string((*imu_buffer)[i].accelX) + " ";
            output += std::to_string((*imu_buffer)[i].accelY) + " ";
            output += std::to_string((*imu_buffer)[i].accelZ) + " ";
            output += std::to_string((*imu_buffer)[i].gravX) + " ";
            output += std::to_string((*imu_buffer)[i].gravY) + " ";
            output += std::to_string((*imu_buffer)[i].gravZ) + " ";

            i++;
            if (i == BUFFER_SIZE) {
                i = 0;
            }
            counter++;
        }
    }
    ofile << output + "\n";
    
}

// This function gets run every time we get a state update from the program.
void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState,
                          IMU_STATE lastImuState, float deltaTime) {
    MOTION_STATE motion = JslGetMotionState(jcHandle);
    int foot = JslGetControllerType(jcHandle);
    if (foot == LEFT_JOYCON) {
    cout << "x: " << motion.gravX << endl;
    cout << "y: " << motion.gravY << endl;
    cout << "z: " << motion.gravZ << endl;
    }
    if (foot == RIGHT_JOYCON) {
    cout << "           x: " << motion.gravX << endl;
    cout << "           y: " << motion.gravY << endl;
    cout << "           z: " << motion.gravZ << endl;
    }

    addFrameToIMUBuffer(imuState, motion, foot);
    if (deltaTime > 0.06) {
        float lagAmount = (deltaTime / 0.015) * 100;
        cout << "WARNING: deltaTime of " << deltaTime * 1000 << " is greater than 30ms on controller "
             << (foot == LEFT_JOYCON ? "LEFT_JOYCON" : "RIGHT_JOYCON") << " (" << lagAmount << "% lag)" << endl;
    }
    /*
    if (motion.accelZ < -1.0) {
        uint64_t currentTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        unsigned int delta = currentTime - lastStomp;
        if (delta > stompBuffer) {
            cout << stompCounter++ << endl;
            cout << "STOMP! Z: " << motion.accelZ << endl;
            lastStomp = currentTime;
            getBuffer();
        }
    }
    */
};

void printbuf(char *buf, int bufsize) {
    printf("buf:");
    for (int i = 0; i < bufsize; i++) {
        printf("%d,", buf[i]);
    }
    printf("\n");
}

// Key press surfaces constants
enum KeyPressSurfaces {
    KEY_PRESS_SURFACE_DEFAULT,
    KEY_PRESS_SURFACE_UP,
    KEY_PRESS_SURFACE_DOWN,
    KEY_PRESS_SURFACE_LEFT,
    KEY_PRESS_SURFACE_RIGHT,
    KEY_PRESS_SURFACE_TOTAL
};

#define UP_BUTTON 5
#define DOWN_BUTTON 3
#define LEFT_BUTTON 4
#define RIGHT_BUTTON 2

void processButtonEvent(SDL_JoyButtonEvent e) {
    cout << unsigned(e.button) << endl;

    switch (e.button) {
        case UP_BUTTON:
        case DOWN_BUTTON:
        case LEFT_BUTTON:
        case RIGHT_BUTTON:
            getBuffer(unsigned(e.button));
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    std::cout << "hi!" << std::endl;
    ofile.open(filename, std::ios::out | std::ios::trunc);

    connectDevices();
    JslSetCallback(&joyShockPollCallback);

    SDL_Joystick *gGameController = NULL;

    // The window we'll be rendering to
    SDL_Window *window = NULL;

    // The surface contained by the window
    SDL_Surface *screenSurface = NULL;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    } else {
        // Create window
        window =
            SDL_CreateWindow("Airdance", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 300, 100, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        } else {
            // Get window surface
            screenSurface = SDL_GetWindowSurface(window);

            // Fill the surface white
            SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

            // Update the surface
            SDL_UpdateWindowSurface(window);

            // Wait two seconds
        }
        // Check for joysticks
        if (SDL_NumJoysticks() < 1) {
            printf("Warning: No joysticks connected!\n");
        } else {
            // Load joystick
            gGameController = SDL_JoystickOpen(0);
            if (gGameController == NULL) {
                printf("Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
            }
        }
    }
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0) {
        throw 1;
    }
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            // User requests quit
            if (event.type == SDL_QUIT) {
                quit = true;
            }

            switch (event.type) {
                case SDL_JOYBUTTONDOWN:
                    processButtonEvent(event.jbutton);
                    break;
                case SDL_JOYBUTTONUP:
                    //processButtonEvent(event.jbutton);
                    break;
            }
        }
    }
    ofile.close();
    cleanup();

    SDL_DestroyWindow(window);
    // Quit SDL subsystems
    SDL_Quit();

    return 0;
}

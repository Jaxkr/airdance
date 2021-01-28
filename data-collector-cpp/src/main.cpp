#include <chrono>
#include <iostream>
#include <mutex>

#include "JoyShockLibrary.h"
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

unsigned int stompBuffer = 60;
uint64_t lastStomp =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
unsigned int stompCounter = 0;

// holds the last 10 samples of IMU data from each foot;
imu_frame_t left_imu_buffer[20];
unsigned int left_imu_index = 0;
mutex m_left_imu_buffer;
imu_frame_t right_imu_buffer[20];
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
    imu_frame_t (*imu_buffer)[20];
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
    if (*imu_index == 20) {
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

This data will be coordinated using the actual stomp pad data. Our data will complete ignore percieved stomps and simply treat the pad as the ultimate source of truth. 

*/


void getBuffer() {
    unsigned int *imu_index;
    imu_frame_t (*imu_buffer)[20];
    imu_index = &left_imu_index;
    imu_buffer = &left_imu_buffer;

    int i = *imu_index;
    int counter = 0;
    cout << "LAST 6 FRAMES" << endl;
    while (counter < 5) {
        cout << "-----------" << endl;
        cout << "ACCEL " << (*imu_buffer)[i].accelX << "|" << (*imu_buffer)[i].accelY << "|" << (*imu_buffer)[i].accelZ << endl;
        cout << "GRAV  " << (*imu_buffer)[i].gravX << "|" << (*imu_buffer)[i].gravY << "|" << (*imu_buffer)[i].gravZ << endl;

        if (i == 19) {
            i = 0;
        } else {
            i++;
        }
        counter++;
    }
}

// This function gets run every time we get a state update from the program.
void joyShockPollCallback(int jcHandle, JOY_SHOCK_STATE state, JOY_SHOCK_STATE lastState, IMU_STATE imuState,
                          IMU_STATE lastImuState, float deltaTime) {
    MOTION_STATE motion = JslGetMotionState(jcHandle);
    int foot = JslGetControllerType(jcHandle);
    // cout << "x: " << motion.gravX << endl;
    // cout << "y: " << motion.gravY << endl;
    // cout << "z: " << motion.gravZ << endl;

    addFrameToIMUBuffer(imuState, motion, foot);
    if (deltaTime > 0.03) {
        float lagAmount = (deltaTime / 0.015) * 100;
        cout << "WARNING: deltaTime of " << deltaTime * 1000 << " is greater than 30ms on controller "
             << (foot == LEFT_JOYCON ? "LEFT_JOYCON" : "RIGHT_JOYCON") << " (" << lagAmount << "% lag)" << endl;
    }

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
};

int main(int argc, char* argv[]) {
    std::cout << "hi!" << std::endl;

    connectDevices();
    JslSetCallback(&joyShockPollCallback);

    // keep the process running
    string enteredCommand;
    bool quit = false;

    while (!quit) {
        getline(cin, enteredCommand);
        cout << enteredCommand;
    }

    return 0;
}

#ifndef SENSOR_EXT_H
#define SENSOR_EXT_H

#include <Arduino.h>

#define TRIG_PIN 5
#define ECHO_PIN 23
#define COLOR_IR_PIN 4
#define SERVO_R_PIN 17
#define SERVO_L_PIN 18

#define CHAN_R 2
#define CHAN_L 3
#define S_FREQ 50
#define S_RES 10

void sensorExtInit();
float getDistance();
void moveServo(int channel, int angle);
void executeGrabProcess();
void executeReleaseProcess();

#endif

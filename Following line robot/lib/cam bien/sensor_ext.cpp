#include "sensor_ext.h"

void sensorExtInit() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(COLOR_IR_PIN, INPUT);

  ledcSetup(CHAN_R, S_FREQ, S_RES);
  ledcAttachPin(SERVO_R_PIN, CHAN_R);

  ledcSetup(CHAN_L, S_FREQ, S_RES);
  ledcAttachPin(SERVO_L_PIN, CHAN_L);
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return -1.0f;
  }
  return duration * 0.034f / 2.0f;
}

static int currentServoAngleL = 90;
static int currentServoAngleR = 90;

void moveServo(int channel, int angle) {
  int duty = map(angle, 0, 180, 20, 130);
  ledcWrite(channel, duty);
  if (channel == CHAN_L) {
    currentServoAngleL = angle;
  } else if (channel == CHAN_R) {
    currentServoAngleR = angle;
  }
}

void executeGrabProcess() {
  Serial.println("He thong: Dang thuc hien quy trinh gap...");
  moveServo(CHAN_L, 135);
  moveServo(CHAN_R, 90);
}

void executeReleaseProcess() {
  Serial.println("He thong: Dang nha vat...");
  moveServo(CHAN_L, 180);
  moveServo(CHAN_R, 45);
}

# 📌 ESP32 Pin Configuration Reference - Final

## Complete Pin Map

```
╔════════════════════════════════════════════════════════════════════╗
║                    ESP32 GPIO Allocation (Refactored)             ║
╚════════════════════════════════════════════════════════════════════╝

┌─ MOTOR CONTROL (L298N/TB6612) ────────────────────────────────────┐
│                                                                     │
│  Left Motor:                                                       │
│  ├─ ENA    (PWM Speed)  → GPIO 27   ← PWM Channel 0               │
│  ├─ IN1    (Direction 1)→ GPIO 14                                 │
│  └─ IN2    (Direction 2)→ GPIO 13                                 │
│                                                                     │
│  Right Motor:                                                      │
│  ├─ ENB    (PWM Speed)  → GPIO 22   ← PWM Channel 1               │
│  ├─ IN3    (Direction 1)→ GPIO 21                                 │
│  └─ IN4    (Direction 2)→ GPIO 19                                 │
│                                                                     │
│  PWM Settings:                                                     │
│  ├─ Frequency: 1000 Hz                                             │
│  ├─ Resolution: 8-bit (0-255)                                     │
│  └─ Speed Range: 0-255 (0 = stop, 255 = max speed)                │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─ LINE SENSORS (QTR-8A Digital) ───────────────────────────────────┐
│                                                                     │
│  D1 → GPIO 39    │    D5 → GPIO 33                                 │
│  D2 → GPIO 36    │    D6 → GPIO 32                                 │
│  D3 → GPIO 26    │    D7 → GPIO 35                                 │
│  D4 → GPIO 25    │    D8 → GPIO 34                                 │
│                                                                     │
│  Sensor Logic:                                                     │
│  ├─ Logic Level: Active-LOW                                       │
│  ├─ Line Detected: LOW (0) → sensorValue = 1                      │
│  ├─ No Line: HIGH (1) → sensorValue = 0                           │
│  └─ Update Rate: ~100+ kHz (direct GPIO, no MUX delay)            │
│                                                                     │
│  Position Calculation:                                             │
│  ├─ Position Range: 0 to 700                                      │
│  ├─ Center Point (Setpoint): 350                                  │
│  ├─ Error Range: -350 to +350                                     │
│  └─ Formula: weightedSum / sensorCount                             │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─ ADDITIONAL SENSORS ──────────────────────────────────────────────┐
│                                                                     │
│  Ultrasonic (HC-SR04):                                             │
│  ├─ TRIG → GPIO 5    (Trigger pulse)                              │
│  ├─ ECHO → GPIO 23   (Echo signal)                                │
│  └─ Function: float getDistance()                                 │
│                                                                     │
│  IR Sensor (Single):                                               │
│  └─ Signal → GPIO 4   (Analog/Digital input)                      │
│                                                                     │
│  Servo Motors:                                                     │
│  ├─ Servo Left → GPIO 18   (PWM 50Hz, duty 20-130)                │
│  └─ Servo Right → GPIO 17  (PWM 50Hz, duty 20-130)                │
│                                                                     │
│  Buzzer:                                                           │
│  └─ ⚠️ DISABLED (was GPIO 5, conflicts with TRIG_PIN)              │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─ CONTROL INTERFACE ───────────────────────────────────────────────┐
│                                                                     │
│  Button Control:                                                   │
│  ├─ Button GND → GPIO 11   (Main control: calibration/start)      │
│  ├─ Button VCC → GPIO 10   (Emergency stop)                       │
│  └─ Debounce: 70ms                                                 │
│                                                                     │
│  Status LEDs:                                                      │
│  ├─ LED ON → GPIO 6   (Mode indicator)                            │
│  └─ LED    → GPIO 7   (Status indicator)                          │
│                                                                     │
│  BLE Communication:                                                │
│  ├─ Onboard ESP32 (GPIO 34/35 RX/TX for serial logging)           │
│  └─ UUID: 0000ffe0-0000-1000-8000-00805f9b34fb                    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

╔════════════════════════════════════════════════════════════════════╗
║                         SUMMARY TABLE                              ║
╚════════════════════════════════════════════════════════════════════╝

GPIO  │ Function           │ Type      │ Direction  │ Notes
──────┼────────────────────┼───────────┼────────────┼─────────────────
4     │ IR Sensor          │ Input     │ ADC/GPIO   │ Single IR
5     │ Ultrasonic TRIG    │ Output    │ HIGH/LOW   │ Trigger pulse
6     │ LED ON             │ Output    │ HIGH/LOW   │ Mode indicator
7     │ LED                │ Output    │ HIGH/LOW   │ Status indicator
10    │ Button VCC         │ Input     │ HIGH/LOW   │ Stop button
11    │ Button GND         │ Input     │ HIGH/LOW   │ Main button
13    │ Motor IN2 (Left)   │ Output    │ HIGH/LOW   │ Direction
14    │ Motor IN1 (Left)   │ Output    │ HIGH/LOW   │ Direction
17    │ Servo Right        │ Output    │ PWM        │ 50Hz, 0-180°
18    │ Servo Left         │ Output    │ PWM        │ 50Hz, 0-180°
19    │ Motor IN4 (Right)  │ Output    │ HIGH/LOW   │ Direction
21    │ Motor IN3 (Right)  │ Output    │ HIGH/LOW   │ Direction
22    │ Motor ENB (PWM)    │ Output    │ PWM        │ Speed control
23    │ Ultrasonic ECHO    │ Input     │ HIGH/LOW   │ Echo signal
25    │ Sensor S4 (D4)     │ Input     │ HIGH/LOW   │ QTR-8A
26    │ Sensor S3 (D3)     │ Input     │ HIGH/LOW   │ QTR-8A
27    │ Motor ENA (PWM)    │ Output    │ PWM        │ Speed control
32    │ Sensor S6 (D6)     │ Input     │ HIGH/LOW   │ QTR-8A
33    │ Sensor S5 (D5)     │ Input     │ HIGH/LOW   │ QTR-8A
34    │ Sensor S8 (D8)     │ Input     │ HIGH/LOW   │ QTR-8A
35    │ Sensor S7 (D7)     │ Input     │ HIGH/LOW   │ QTR-8A
36    │ Sensor S2 (D2)     │ Input     │ HIGH/LOW   │ QTR-8A
39    │ Sensor S1 (D1)     │ Input     │ HIGH/LOW   │ QTR-8A
```

---

## Motor Direction Truth Table (L298N)

### Left Motor (ENA + IN1/IN2)

| ENA | IN1 | IN2 | Result | Speed |
|-----|-----|-----|--------|-------|
| PWM | LOW | HIGH | Forward | 0-255 |
| PWM | HIGH | LOW | Backward | 0-255 |
| PWM | LOW | LOW | Stop | 0 |
| PWM | HIGH | HIGH | Stop | 0 |
| 0 | X | X | Stop | 0 |

### Right Motor (ENB + IN3/IN4)

| ENB | IN3 | IN4 | Result | Speed |
|-----|-----|-----|--------|-------|
| PWM | LOW | HIGH | Forward | 0-255 |
| PWM | HIGH | LOW | Backward | 0-255 |
| PWM | LOW | LOW | Stop | 0 |
| PWM | HIGH | HIGH | Stop | 0 |
| 0 | X | X | Stop | 0 |

---

## Sensor Reading Logic

### QTR-8A Digital Mode

```
GPIO State  →  Sensor Value  →  Line Detection
─────────────────────────────────────────────
LOW (0)     →  1             →  ✓ Line detected
HIGH (1)    →  0             →  ✗ No line
```

### Position Calculation

```
Sensor Array:  [D1] [D2] [D3] [D4] [D5] [D6] [D7] [D8]
Position:        0   100  200  300  400  500  600  700
Center:                    350 (setpoint)
Error = Current Position - 350
```

---

## PWM Specifications

### Motor Speed Control (1000Hz)

```
Speed Value  →  Duty Cycle  →  PWM Output
─────────────────────────────────────────
0            →  0%          →  Stop
85           →  33%         →  Base speed
127          →  50%         →  Half speed
180          →  71%         →  High speed
255          →  100%        →  Max speed
```

### Servo Control (50Hz)

```
Angle (°)  →  Duty Cycle  →  Pulse Width
─────────────────────────────────────────
0          →  7.7%        →  ~0.77ms
90         →  29%         →  ~2.9ms
180        →  50%         →  ~5.0ms

Mapping Formula: duty = map(angle, 0, 180, 20, 130)
```

---

## Hardware Connection Diagram

```
[ESP32 Board]
     │
     ├─→ [L298N Motor Driver]
     │    ├─ GPIO27 (ENA) ──→ PWM IN1A (Left Speed)
     │    ├─ GPIO14 (IN1) ──→ IN2A (Left Dir1)
     │    ├─ GPIO13 (IN2) ──→ IN1A (Left Dir2)
     │    ├─ GPIO22 (ENB) ──→ PWM IN3A (Right Speed)
     │    ├─ GPIO21 (IN3) ──→ IN4A (Right Dir1)
     │    └─ GPIO19 (IN4) ──→ IN3A (Right Dir2)
     │         │
     │         ├─→ [Left Motor] ←→ [Left Wheel]
     │         └─→ [Right Motor] ←→ [Right Wheel]
     │
     ├─→ [QTR-8A Sensor Module]
     │    ├─ GPIO39 (S1/D1) ──→ Sensor 1
     │    ├─ GPIO36 (S2/D2) ──→ Sensor 2
     │    ├─ GPIO26 (S3/D3) ──→ Sensor 3
     │    ├─ GPIO25 (S4/D4) ──→ Sensor 4
     │    ├─ GPIO33 (S5/D5) ──→ Sensor 5
     │    ├─ GPIO32 (S6/D6) ──→ Sensor 6
     │    ├─ GPIO35 (S7/D7) ──→ Sensor 7
     │    └─ GPIO34 (S8/D8) ──→ Sensor 8
     │
     ├─→ [HC-SR04 Ultrasonic]
     │    ├─ GPIO5 (TRIG) ──→ Trigger
     │    └─ GPIO23 (ECHO) ──→ Echo
     │
     ├─→ [IR Sensor]
     │    └─ GPIO4 ──→ Signal
     │
     ├─→ [Servo Motors]
     │    ├─ GPIO18 ──→ Servo Left
     │    └─ GPIO17 ──→ Servo Right
     │
     └─→ [Control Interface]
          ├─ GPIO11 ──→ Button GND
          ├─ GPIO10 ──→ Button VCC
          ├─ GPIO6 ──→ LED ON
          └─ GPIO7 ──→ LED
```

---

## Quick Reference Commands

### Motor Control (C++)

```cpp
// Forward
setMotor(200, 200);  // Both motors 200/255

// Backward
setMotor(-200, -200);

// Stop
stopMotors();  // or setMotor(0, 0)

// Turn Left
setMotor(-150, 150);

// Turn Right
setMotor(150, -150);
```

### Sensor Reading

```cpp
readAllSensors();  // Read all 8 sensors into sensorValues[]
linePosition = calculateLinePosition();
```

### BLE Commands (JSON)

```json
{"cmd": "START_ROBOT"}
{"cmd": "STOP_ROBOT"}
{"cmd": "SET_PID", "kp": 3.0, "ki": 0.02, "kd": 4.0}
{"cmd": "SET_SPEEDS", "base": 85, "max": 180, "turn": 220}
{"cmd": "MANUAL_CONTROL", "left": 100, "right": 150}
```

---

## Troubleshooting

| Symptom | Possible Cause | Check |
|---------|----------------|-------|
| Motors don't move | IN pins LOW/HIGH issue | Motor truth table above |
| Only one motor moves | PWM channel not attached | `ledcAttachPin()` |
| No line detection | Sensor pins wrong | GPIO 25,26,32-36,39 |
| Wrong direction | IN1/IN2 or IN3/IN4 swapped | Reverse pin logic |
| Servo doesn't move | PWM not configured | Check GPIO 17,18 |
| Ultrasonic fails | TRIG (GPIO 5) conflict | Buzzer disabled? |
| BLE disconnects | Button interrupt issue | Check GPIO 10,11 |

---

## Configuration Summary

| Parameter | Value |
|-----------|-------|
| Motor Driver | L298N/TB6612 |
| Motor PWM Freq | 1000 Hz |
| Motor Speed Range | 0-255 |
| Sensor Type | QTR-8A Digital |
| Sensor Count | 8 |
| Sensor Speed | ~100+ kHz |
| Servo PWM Freq | 50 Hz |
| Button Debounce | 70ms |
| BLE Device Name | LineFollowing_Robot |

---

**Last Updated:** May 8, 2026  
**Version:** 2.0 (Refactored with QTR-8A)  
**Status:** ✅ Ready for hardware testing

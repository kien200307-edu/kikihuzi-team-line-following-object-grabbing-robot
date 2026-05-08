# 📌 Quick Pin Reference - Robot Refactor

## ESP32 Pin Configuration (New)

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 PIN LAYOUT                        │
├─────────────────────────────────────────────────────────────┤
│ MOTOR CONTROL (L298N/TB6612)                               │
│  ENA (Left PWM)   → GPIO27                                 │
│  IN1 (Left Dir1)  → GPIO14                                 │
│  IN2 (Left Dir2)  → GPIO13                                 │
│  ENB (Right PWM)  → GPIO22                                 │
│  IN3 (Right Dir1) → GPIO21                                 │
│  IN4 (Right Dir2) → GPIO19                                 │
├─────────────────────────────────────────────────────────────┤
│ LINE SENSORS (QTR-8A - Digital Mode)                       │
│  S1 (D1) → GPIO39  │  S5 (D5) → GPIO33                    │
│  S2 (D2) → GPIO36  │  S6 (D6) → GPIO32                    │
│  S3 (D3) → GPIO26  │  S7 (D7) → GPIO35                    │
│  S4 (D4) → GPIO25  │  S8 (D8) → GPIO34                    │
├─────────────────────────────────────────────────────────────┤
│ SERVO CONTROL (PWM - 50Hz)                                 │
│  Servo Left   → GPIO18 (Channel 3)                         │
│  Servo Right  → GPIO17 (Channel 2)                         │
├─────────────────────────────────────────────────────────────┤
│ ULTRASONIC SENSOR (HC-SR04)                                │
│  TRIG → GPIO5                                              │
│  ECHO → GPIO23                                             │
├─────────────────────────────────────────────────────────────┤
│ IR SENSOR                                                   │
│  IR Signal → GPIO4                                         │
├─────────────────────────────────────────────────────────────┤
│ COMMUNICATION                                               │
│  Serial (UART0) → Default (already configured)             │
│  BLE → Built-in (wireless)                                 │
└─────────────────────────────────────────────────────────────┘
```

---

## Motor Control Truth Table (L298N/TB6612)

### Left Motor (ENA + IN1/IN2)

| IN1 | IN2 | Action |
|-----|-----|--------|
| LOW | HIGH | Forward (CW) |
| HIGH | LOW | Backward (CCW) |
| LOW | LOW | Stop |
| HIGH | HIGH | Stop |

**Speed Control:** ENA (PWM 0-4095) → Motor Speed (0-255 equivalent)

### Right Motor (ENB + IN3/IN4)

| IN3 | IN4 | Action |
|-----|-----|--------|
| LOW | HIGH | Forward (CW) |
| HIGH | LOW | Backward (CCW) |
| LOW | LOW | Stop |
| HIGH | HIGH | Stop |

**Speed Control:** ENB (PWM 0-4095) → Motor Speed (0-255 equivalent)

---

## Sensor Reading Mode

### QTR-8A (Digital Mode)
- **Pins:** GPIO39, GPIO36, GPIO26, GPIO25, GPIO33, GPIO32, GPIO35, GPIO34
- **Logic:** Active-LOW (LOW = line detected, HIGH = no line)
- **Update Rate:** ~200Hz (ISR timer)
- **Debounce:** 2ms delay

### Calculation
```cpp
Pattern = [D1 D2 D3 D4 D5 D6 D7 D8]  (8-bit)
Error = weighted_sum / sensor_count
Weights = [-35, -25, -15, -5, 5, 15, 25, 35] / 10
Range: -3.5 to +3.5
```

---

## PID Control Values

```cpp
Kp (Proportional) = 3.0f   // Adjust via BLE
Kd (Derivative)   = 18.0f  // Adjust via BLE
Base Speed        = 180    // Adjust via BLE
Max Speed         = 255
Watchdog Timer    = 2000ms // Stop if no command
```

### Motor Output Calculation
```cpp
correction = Kp * error + Kd * (error - lastError)
left_speed = baseSpeed - correction
right_speed = baseSpeed + correction
```

---

## Servo Control

### Angles & Pulse Width
| Angle | Duty Cycle |
|-------|-----------|
| 0° | 20 |
| 90° | 75 |
| 180° | 130 |

### Functions
```cpp
moveServo(CHAN_L, angle);  // Servo Left (GPIO18)
moveServo(CHAN_R, angle);  // Servo Right (GPIO17)

// Pre-defined:
executeGrabProcess();      // Grab position
executeReleaseProcess();   // Release position
```

---

## Ultrasonic Distance Measurement

**Formula:** `distance = duration × 0.034 / 2` (cm)

**Max Range:** 30000μs timeout = ~510cm

**Function:** `float getDistance()`

---

## BLE Commands

### Command Format (JSON)

**Auto Mode Control:**
```
AUTO:ON    // Automatic line following
AUTO:OFF   // Manual mode
STOP       // Emergency stop
```

**PID Tuning:**
```
PID P=3.5 D=20.0 S=190
// P: Kp (0-50)
// D: Kd (0-100)
// S: BaseSpeed (80-255)
```

---

## File Structure

```
Following line robot/
├── src/
│   └── main.cpp              ← UPDATED ✅
├── lib/
│   └── cam bien/
│       ├── sensor_ext.h      ← UPDATED ✅
│       ├── sensor_ext.cpp
│       ├── motor.cpp
│       └── ...
├── include/
├── platformio.ini
└── REFACTOR_SUMMARY.md       ← NEW 📄
```

---

## Compilation

**Platform:** ESP32  
**Framework:** Arduino  
**Board:** ESP32 Dev Kit (or similar)

**Compile Command:**
```bash
platformio run -e esp32
```

**Upload Command:**
```bash
platformio run -e esp32 --target upload
```

---

## Hardware Wiring Checklist

- [ ] Motor Left connected to ENA (27), IN1 (14), IN2 (13)
- [ ] Motor Right connected to ENB (22), IN3 (21), IN4 (19)
- [ ] QTR-8A sensors connected to GPIO39, 36, 26, 25, 33, 32, 35, 34
- [ ] Servo Left connected to GPIO18
- [ ] Servo Right connected to GPIO17
- [ ] Ultrasonic TRIG → GPIO5, ECHO → GPIO23
- [ ] IR Sensor → GPIO4
- [ ] Power supply appropriate for motors
- [ ] ESP32 powered correctly (5V or 3.3V + GND)

---

## 🚀 Quick Start

1. **Compile & Upload:**
   ```bash
   platformio run -e esp32 --target upload
   ```

2. **Monitor Serial Output:**
   ```bash
   platformio device monitor
   ```

3. **Connect via BLE:**
   - Use BLE terminal or custom app
   - Connect to device name: "LINE_ROBOT"
   - Send: `AUTO:ON` to start line following

4. **Calibrate if needed:**
   - Adjust Kp/Kd via BLE command
   - Monitor error and motor speeds in Serial

5. **Test Components:**
   - Check motor directions
   - Verify sensor readings
   - Test servo positions

---

**Last Updated:** 2026-05-08
**Version:** 2.0 (Refactored with QTR-8A)

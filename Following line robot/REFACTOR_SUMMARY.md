# Refactor Summary - Line Following Robot

## Tổng Quan
Code robot dò line đã được refactor để tương thích với cấu hình phần cứng mới. Toàn bộ logic điều khiển và giao diện BLE vẫn được giữ nguyên.

---

## 📋 Các Thay Đổi Chi Tiết

### 1. **Pin Mapping - Motor (L298N/TB6612)**
| Chức Năng | Pin Cũ | Pin Mới |
|-----------|--------|---------|
| Left Motor PWM (ENA) | 25 | 27 |
| Left Motor Dir 1 (IN1) | 26 | 14 |
| Left Motor Dir 2 (IN2) | 27 | 13 |
| Right Motor PWM (ENB) | 14 | 22 |
| Right Motor Dir 1 (IN3) | 33 | 21 |
| Right Motor Dir 2 (IN4) | 13 | 19 |

### 2. **Pin Mapping - Line Sensors (QTR-8A)**
| Sensor | Pin Cũ | Pin Mới | Ghi Chú |
|--------|--------|---------|---------|
| S1 (D1) | 32 | 39 | - |
| S2 (D2) | 35 | 36 | - |
| S3 (D3) | 34 | 26 | - |
| S4 (D4) | 39 | 25 | - |
| S5 (D5) | 36 | 33 | - |
| S6 (D6) | - | 32 | **Mới** |
| S7 (D7) | - | 35 | **Mới** |
| S8 (D8) | - | 34 | **Mới** |

**Nâng cấp:** Từ 5 cảm biến → 8 cảm biến QTR-8A

### 3. **Pin Mapping - Servo**
| Chức Năng | Pin Cũ | Pin Mới |
|-----------|--------|---------|
| Servo Trái | 19 | 18 |
| Servo Phải | 18 | 17 |

### 4. **Pin Mapping - Ultrasonic (HC-SR04)**
| Chức Năng | Pin Cũ | Pin Mới |
|-----------|--------|---------|
| TRIG | 5 | 5 (giữ nguyên) |
| ECHO | 21 | 23 |

### 5. **Pin Mapping - IR Sensor**
| Chức Năng | Pin Cũ | Pin Mới |
|-----------|--------|---------|
| IR Signal | 4 | 4 (giữ nguyên) |

---

## 🔧 Thay Đổi Logic Code

### A. **Cảm Biến Dò Line**

#### Thay đổi hàm `readPattern()`
- **Trước:** 5-bit pattern (bit4-bit0 cho S1-S5)
- **Sau:** 8-bit pattern (bit7-bit0 cho S1-S8)
```cpp
// Trước: return ((!digitalRead(S1)) << 4) | ... | ((!digitalRead(S5)));
// Sau:   return ((!digitalRead(S1)) << 7) | ... | ((!digitalRead(S8)));
```

#### Thay đổi hàm `readError()`
- **Trước:** Weights: [-2, -1, 0, 1, 2] cho 5 sensor, phạm vi error: -2 to +2
- **Sau:** Weights: [-35, -25, -15, -5, 5, 15, 25, 35] (scaled by 10) cho 8 sensor, phạm vi error: -3.5 to +3.5
- **Lợi ích:** Độ chính xác cao hơn, phản ứng tốt hơn với các đường line
```cpp
const int weights[8] = { -35, -25, -15, -5, 5, 15, 25, 35 };
// Error được tính toán với độ chính xác cao hơn
```

#### Thay đổi ISR `onTimer()`
- **Trước:** Build 5-bit pattern
- **Sau:** Build 8-bit pattern
- Kiểm tra tất cả 8 sensor thay vì 5 sensor

### B. **Biến Toàn Cục**
```cpp
// Trước:
volatile unsigned char sensor = 0;   // 5-bit pattern

// Sau:
volatile uint16_t sensor = 0;        // 8-bit pattern
```

---

## 📁 Các File Được Sửa

### 1. **src/main.cpp**
- ✅ Cập nhật tất cả pin definitions
- ✅ Cập nhật hàm `readPattern()` cho 8 sensors
- ✅ Cập nhật hàm `readError()` với weights mới
- ✅ Cập nhật ISR `onTimer()`
- ✅ Cập nhật `setup()` để init pins mới
- ✅ Giữ nguyên toàn bộ logic BLE
- ✅ Giữ nguyên logic PID/điều khiển motor

### 2. **lib/cam bien/sensor_ext.h**
- ✅ Cập nhật `ECHO_PIN` từ 21 → 23
- ✅ Cập nhật `SERVO_R_PIN` từ 18 → 17  
- ✅ Cập nhật `SERVO_L_PIN` từ 19 → 18
- ✅ Giữ nguyên `TRIG_PIN = 5`
- ✅ Giữ nguyên `COLOR_IR_PIN = 4`

---

## 🔄 Các Thành Phần Được Giữ Nguyên

✅ **BLE Communication**
- UUID của service và characteristic
- Command parsing
- Response handling
- BLE Callbacks

✅ **Motor Control Logic**
- `setMotor()` function
- Motor PWM mapping (0-255 → 0-4095)
- Motor direction control
- Watchdog mechanism

✅ **PID Algorithm**
- PID parameters (Kp, Ki, Kd - hiện tại chỉ có Kp, Kd)
- Base speed adjustments
- Speed limiting
- Error tracking

✅ **Operating Modes**
- LINE mode
- MANUAL mode
- Mode switching logic

✅ **Servo & Ultrasonic Functions**
- `getDistance()` từ HC-SR04
- `moveServo()` for servo control
- `executeGrabProcess()` and `executeReleaseProcess()`

---

## 🧪 Testing Checklist

### Motor Testing
- [ ] Left motor forward/backward
- [ ] Right motor forward/backward
- [ ] Motor PWM mapping correctly (0-255 → PWM)
- [ ] Direction control with IN1-4 pins

### Sensor Testing
- [ ] QTR-8A sensor readings (D1-D8)
- [ ] All 8 sensors detect line
- [ ] Debounce working correctly (2ms delay)
- [ ] Center position calculation for 8 sensors

### PID Testing
- [ ] Error calculation with new weights
- [ ] Motor correction values
- [ ] Line following stability
- [ ] Speed adjustments based on error

### Servo Testing
- [ ] Servo left (pin 18) movement
- [ ] Servo right (pin 17) movement
- [ ] Grab/release processes

### Ultrasonic Testing
- [ ] TRIG (pin 5) pulse generation
- [ ] ECHO (pin 23) pulse reading
- [ ] Distance calculation

### IR Sensor Testing
- [ ] IR sensor reading (pin 4)

### BLE Testing
- [ ] BLE connection
- [ ] Command reception (AUTO:ON, AUTO:OFF, PID, STOP)
- [ ] Mode switching
- [ ] Status feedback

---

## 📝 Notes

1. **QTR-8A Sensors**: Code sử dụng digital mode (active-LOW). Đảm bảo jumper trên QTR-8A module được cấu hình đúng.

2. **PWM Frequency**: Giữ nguyên 1000Hz cho motor PWM, 50Hz cho servo PWM.

3. **Error Range**: Với 8 sensors, error range mở rộng từ ±2 (5 sensors) thành ±3.5 (8 sensors). Có thể cần điều chỉnh Kp/Kd tùy theo phản ứng thực tế.

4. **EEPROM**: Vẫn được initialize nhưng chưa sử dụng. Có thể dùng trong tương lai để lưu calibration data.

5. **Sensor Offset**: Với 8 sensors, setpoint thay đổi từ (5-1)*100/2 = 200 thành (8-1)*100/2 = 350. Code hiện tại không sử dụng setpoint cụ thể, chỉ sử dụng error từ center.

---

## ⚠️ Lưu Ý Quan Trọng

- Kiểm tra **độ ổn định** khi robot chạy theo line với 8 sensors
- Có thể cần **điều chỉnh Kp, Kd** thông qua BLE để tối ưu hóa
- **Test phần cứng** trước khi hoạt động full-scale
- Đảm bảo các **điện trở pull-up/down** được kết nối đúng cho sensors

---

## 🎯 Tính Năng Được Hỗ Trợ

✅ Dò line tự động với 8 cảm biến  
✅ Điều khiển thông BLE (điện thoại/web app)  
✅ Điều khiển servo grab/release  
✅ Đo khoảng cách ultrasonic  
✅ Cảm biến hồng ngoại  
✅ PID control động  
✅ Watchdog timer  
✅ EEPROM support  

---

**Status:** ✅ Refactor Complete & Compiled Successfully

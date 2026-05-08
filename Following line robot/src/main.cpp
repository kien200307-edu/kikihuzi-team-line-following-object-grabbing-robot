#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ========= BLE CONFIGURATION =========
#define SERVICE_UUID          "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID   "0000ffe1-0000-1000-8000-00805f9b34fb"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
bool _BLEClientConnected = false;

// ========= L298N/TB6612 MOTOR DRIVER PINS =========
#define ENA 27       // Left motor PWM
#define IN1 14       // Left motor direction 1
#define IN2 13       // Left motor direction 2
#define ENB 22       // Right motor PWM
#define IN3 21       // Right motor direction 1
#define IN4 19       // Right motor direction 2

// PWM Channels for L298N
#define PWM_CHANNEL_LEFT  0
#define PWM_CHANNEL_RIGHT 1

// PWM Settings
#define PWM_FREQ 1000   // 1kHz frequency
#define PWM_RES 8       // 8-bit resolution (0-255)

// ========= BUTTON CONTROL PINS =========
#define BUTTON_GND_PIN 11  // GPIO11: nút kéo xuống GND - Main Control
#define BUTTON_VCC_PIN 10  // GPIO10: nút kéo lên VCC - Stop Button
#define DEBOUNCE_DELAY 70

// Other pins
#define LED_ON   6
#define LED      7

// ========= QTR-8A LINE SENSOR PINS (Digital) =========
#define S1 39      // D1
#define S2 36      // D2
#define S3 26      // D3
#define S4 25      // D4
#define S5 33      // D5
#define S6 32      // D6
#define S7 35      // D7
#define S8 34      // D8

// ========= ADDITIONAL SENSORS =========
#define TRIG_PIN 5   // Ultrasonic TRIG
#define ECHO_PIN 23  // Ultrasonic ECHO
#define IR_PIN 4     // IR Sensor
#define SERVO_LEFT  18   // Servo left
#define SERVO_RIGHT 17   // Servo right

// ========= SENSOR CONFIGURATION =========
#define NUM_SENSORS 8
#define CALIBRATION_SAMPLES 150 // 100
#define MIN_SENSOR_VALUE 0
#define MAX_SENSOR_VALUE 1     // Digital: 0 or 1
#define SENSOR_THRESHOLD_OFFSET 0  // Digital mode, no offset needed

// ========= PID CONFIGURATION =========
#define DEFAULT_KP 1.2 // 1
#define DEFAULT_KI 0.02 // 0.02 
#define DEFAULT_KD 4   // 3 
#define BASE_SPEED 85 // 70  
#define MAX_SPEED 180   // 120
#define TURN_SPEED 220// 170

#define BOOST_SPEED 130       // Tốc độ tăng cường khi đi thẳng
#define STRAIGHT_THRESHOLD 60  // Ngưỡng để coi là đi thẳng (±50 từ setpoint)
#define BOOST_ENABLE_COUNT 3  // Số lần liên tiếp ở vùng thẳng mới boost
#define SPEED_STEP 15           // Tăng/giảm 8 đơn vị mỗi lần
#define SPEED_CHANGE_DELAY 2   // Thay đổi mỗi 2 loop cycles

// ========= GLOBAL VARIABLES =========

// global variables for line following
int speedChangeCounter = 0;
int straightLineCounter = 0;
int previousBaseSpeed = BASE_SPEED;

// Button variables
volatile bool button32Pressed = false;
volatile bool button19Pressed = false;
volatile unsigned long lastDebounceTime32 = 0;
volatile unsigned long lastDebounceTime19 = 0;

// Control mode variables
enum RobotMode {
  MODE_IDLE,
  MODE_BLE,
  MODE_MANUAL_CALIBRATE_WHITE,
  MODE_MANUAL_CALIBRATE_BLACK,
  MODE_MANUAL_RUNNING
};

RobotMode currentMode = MODE_IDLE;
int button32PressCount = 0;
bool button32StateChanged = false;
bool button19StateChanged = false;

// Sensor arrays
int sensorValues[NUM_SENSORS];        // Current raw sensor readings
int whiteValues[NUM_SENSORS];         // White calibration values
int blackValues[NUM_SENSORS];         // Black calibration values
int thresholdValues[NUM_SENSORS];     // Calculated thresholds
bool digitalSensors[NUM_SENSORS];     // Digital sensor states (0=white, 1=black)

// PID variables
float kp = DEFAULT_KP;
float ki = DEFAULT_KI;
float kd = DEFAULT_KD;
int baseSpeed = BASE_SPEED;
int maxSpeed = MAX_SPEED;
int turnSpeed = TURN_SPEED;
int boostSpeed = BOOST_SPEED;
int straightThreshold = STRAIGHT_THRESHOLD;
int boostEnableCount = BOOST_ENABLE_COUNT;
int speedStep = SPEED_STEP;
int speedChangeDelay = SPEED_CHANGE_DELAY;
int sensorThresholdOffset = SENSOR_THRESHOLD_OFFSET;

int lastError = 0;
int integral = 0;
int setpoint = (NUM_SENSORS - 1) * 100 / 2; // Center position (350 for 8 sensors)

// Line position variables
int linePosition = 0;
bool lineDetected = false;
int lastValidPosition = setpoint;

// Robot state
bool robotRunning = false;
bool isCalibrated = false;
String robotStatus = "IDLE";


void processBlECommand(String jsonStr);
void sendResponse(String status, String message);
void sendRobotStatus();
void sendSensorData();
void updateBLEStatus();
void calibrateWhiteSensors();
void calibrateBlackSensors();
void calculateThresholds();
void readAllSensors();
void setMotorSpeeds(int leftSpeed, int rightSpeed);
void stopAllMotors();
void followLine();
void pidControl();
void printSensorValues();
void printCalibrationData();

void sendResponse(String status, String message) {
  if (!_BLEClientConnected) return;
  
  DynamicJsonDocument doc(512);
  doc["type"] = "response";
  doc["status"] = status;
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  pCharacteristic->setValue(jsonString.c_str());
  pCharacteristic->notify();
}
  
  // ========= BLE COMMAND PROCESSING =========
void processBlECommand(String jsonStr) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonStr);
  
  if (error) {
    Serial.print("❌ JSON parse error: ");
    Serial.println(error.c_str());
    sendResponse("ERROR", "Invalid JSON format");
    return;
  }
  
  String cmd = doc["cmd"].as<String>();
  cmd.toUpperCase();
  
  if (cmd == "GET_STATUS") {
    sendRobotStatus();
    
  } else if (cmd == "SET_PID") {
    if (doc.containsKey("kp")) kp = doc["kp"].as<float>();
    if (doc.containsKey("ki")) ki = doc["ki"].as<float>();
    if (doc.containsKey("kd")) kd = doc["kd"].as<float>();
    
    Serial.printf("🎛️ PID updated: Kp=%.3f Ki=%.3f Kd=%.3f\n", kp, ki, kd);
    String msg = "PID parameters updated : kp=" + String(kp, 3) + 
             ", ki=" + String(ki, 3) + 
             ", kd=" + String(kd, 3);
    sendResponse("SUCCESS", msg);

  } else if (cmd == "SET_SPEEDS") {
    if (doc.containsKey("base")) baseSpeed = doc["base"];
    if (doc.containsKey("max")) maxSpeed = doc["max"];
    if (doc.containsKey("turn")) turnSpeed = doc["turn"];
    if (doc.containsKey("boost")) boostSpeed = doc["boost"];
    
    Serial.printf("⚡ Speeds updated: Base=%d Max=%d Turn=%d Boost=%d\n", 
                  baseSpeed, maxSpeed, turnSpeed, boostSpeed);
    sendResponse("SUCCESS", "Speed parameters updated");
    
  } else if (cmd == "SET_ADVANCED") {
    if (doc.containsKey("straightThreshold")) straightThreshold = doc["straightThreshold"];
    if (doc.containsKey("boostEnableCount")) boostEnableCount = doc["boostEnableCount"];
    if (doc.containsKey("speedStep")) speedStep = doc["speedStep"];
    if (doc.containsKey("speedChangeDelay")) speedChangeDelay = doc["speedChangeDelay"];
    if (doc.containsKey("thresholdOffset")) sensorThresholdOffset = doc["thresholdOffset"];
    
    Serial.println("🔧 Advanced parameters updated");
    sendResponse("SUCCESS", "Advanced parameters updated");
    
  } else if (cmd == "START_CALIBRATION") {
    String type = doc["type"].as<String>();
    if (type == "white") {
      Serial.println("🟦 BLE: Starting white calibration...");
      calibrateWhiteSensors();
      sendResponse("SUCCESS", "White calibration completed");
    } else if (type == "black") {
      Serial.println("⬛ BLE: Starting black calibration...");
      calibrateBlackSensors();
      calculateThresholds();
      sendResponse("SUCCESS", "Black calibration completed");
    }
    
  } else if (cmd == "START_ROBOT") {
    if (!isCalibrated) {
      sendResponse("ERROR", "Robot not calibrated");
      return;
    }
    robotRunning = true;
    robotStatus = "RUNNING";
    Serial.println("🚀 BLE: Robot started");
    sendResponse("SUCCESS", "Robot started");
    
  } else if (cmd == "STOP_ROBOT") {
    robotRunning = false;
    stopAllMotors();
    robotStatus = "STOPPED";
    Serial.println("🛑 BLE: Robot stopped");
    sendResponse("SUCCESS", "Robot stopped");
    
  } else if (cmd == "RESET_ROBOT") {
    robotRunning = false;
    stopAllMotors();
    currentMode = MODE_BLE;
    robotStatus = "IDLE";
    Serial.println("🔄 BLE: Robot reset");
    sendResponse("SUCCESS", "Robot reset");
    
  } else if (cmd == "MANUAL_CONTROL") {
    int leftSpeed = doc["left"].as<int>();
    int rightSpeed = doc["right"].as<int>();
    
    robotRunning = false; // Stop line following
    setMotorSpeeds(leftSpeed, rightSpeed);
    
    Serial.printf("🎮 Manual control: L=%d R=%d\n", leftSpeed, rightSpeed);
    // sendResponse("SUCCESS", "Manual control applied");
    
  } else if (cmd == "GET_SENSORS") {
    readAllSensors();
    sendSensorData();
    
  } else {
    Serial.println("❓ Unknown command: " + cmd);
    sendResponse("ERROR", "Unknown command");
  }
}



void sendRobotStatus() {
  if (!_BLEClientConnected) return;
  
  DynamicJsonDocument doc(1024);
  doc["type"] = "status";
  doc["mode"] = currentMode;
  doc["running"] = robotRunning;
  doc["calibrated"] = isCalibrated;
  doc["status"] = robotStatus;
  
  // PID Parameters
  doc["pid"]["kp"] = kp;
  doc["pid"]["ki"] = ki;
  doc["pid"]["kd"] = kd;
  
  // Speed Parameters
  doc["speeds"]["base"] = baseSpeed;
  doc["speeds"]["max"] = maxSpeed;
  doc["speeds"]["turn"] = turnSpeed;
  doc["speeds"]["boost"] = boostSpeed;
  
  // Advanced Parameters
  doc["advanced"]["straightThreshold"] = straightThreshold;
  doc["advanced"]["boostEnableCount"] = boostEnableCount;
  doc["advanced"]["speedStep"] = speedStep;
  doc["advanced"]["speedChangeDelay"] = speedChangeDelay;
  doc["advanced"]["thresholdOffset"] = sensorThresholdOffset;
  
  // Current sensor data if calibrated
  if (isCalibrated) {
    doc["linePosition"] = linePosition;
    doc["lineDetected"] = lineDetected;
    doc["lastError"] = lastError;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  pCharacteristic->setValue(jsonString.c_str());
  pCharacteristic->notify();
}

  // ========= MOTOR CONTROL FUNCTIONS =========
void motor1Drive(int speed) {
  speed = constrain(speed, -255, 255);
  
  if (speed > 0) {
    // Forward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(PWM_CHANNEL_LEFT, speed);
  } else if (speed < 0) {
    // Backward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(PWM_CHANNEL_LEFT, -speed);
  } else {
    // Stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(PWM_CHANNEL_LEFT, 0);
  }
}

void motor2Drive(int speed) {
  speed = constrain(speed, -255, 255);
  
  if (speed > 0) {
    // Forward
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(PWM_CHANNEL_RIGHT, speed);
  } else if (speed < 0) {
    // Backward
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_RIGHT, -speed);
  } else {
    // Stop
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_RIGHT, 0);
  }
}

void stopAllMotors() {
  motor1Drive(0);  // Left motor
  motor2Drive(0);  // Right motor
}


// ========= BLE CALLBACKS =========
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    _BLEClientConnected = true;
    Serial.println("📱 BLE Client Connected");
    currentMode = MODE_BLE;    // Send initial status
    sendRobotStatus();
  }



  void onDisconnect(BLEServer* pServer) {
    _BLEClientConnected = false;
    Serial.println("📱 BLE Client Disconnected");
    
    // Return to idle mode when disconnected
    if (currentMode == MODE_BLE) {
      currentMode = MODE_IDLE;
      robotRunning = false;
      stopAllMotors();
    }
    
    pServer->getAdvertising()->start();
    Serial.println("📡 BLE Advertising restarted");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() == 0) return;

    String receivedData = String(rxValue.c_str());
    Serial.print("📥 Received: ");
    Serial.println(receivedData);

    // Parse JSON command
    processBlECommand(receivedData);
  }
};

void sendSensorData() {
  if (!_BLEClientConnected) return;
  
  DynamicJsonDocument doc(2048);
  doc["type"] = "sensors";
  
  JsonArray rawSensors = doc.createNestedArray("raw");
  JsonArray digitalSens = doc.createNestedArray("digital");
  JsonArray whiteCal = doc.createNestedArray("white");
  JsonArray blackCal = doc.createNestedArray("black");
  JsonArray thresholds = doc.createNestedArray("thresholds");
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    rawSensors.add(sensorValues[i]);
    digitalSens.add(digitalSensors[i]);
    whiteCal.add(whiteValues[i]);
    blackCal.add(blackValues[i]);
    thresholds.add(thresholdValues[i]);
  }
  
  doc["linePosition"] = linePosition;
  doc["lineDetected"] = lineDetected;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  pCharacteristic->setValue(jsonString.c_str());
  pCharacteristic->notify();
}

void InitBLE() {
  Serial.println("🔵 Initializing BLE...");
  
  BLEDevice::init("kikihuzi");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  pServer->getAdvertising()->start();

  Serial.println("✅ BLE initialized and advertising started");
  Serial.println("📱 Device name: kikihuzi");
}

// ========= PERIODIC BLE UPDATES =========
void updateBLEStatus() {
  static unsigned long lastBLEUpdate = 0;
  
  if (_BLEClientConnected && currentMode == MODE_BLE) {
    // Send status every 2 seconds
    if (millis() - lastBLEUpdate > 5000) {  // ban đầu là 2000ms nhưng thấy lâu quá nên đổi thành 5000ms hẹ hẹ 
      sendRobotStatus();
      lastBLEUpdate = millis();
    }
  }
}

// ========= FORWARD DECLARATIONS =========
void initPins();
void initMotors();
void motor1Drive(int speed);
void motor2Drive(int speed);
void stopAllMotors();
void setMotorSpeeds(int leftSpeed, int rightSpeed);
void readAllSensors();
void calibrateWhiteSensors();
void calibrateBlackSensors();
void calculateThresholds();
void digitizeSensors();
int calculateLinePosition();
void pidControl();
void followLine();
void processButtonEvents();
void printSensorValues();
void printCalibrationData();
void IRAM_ATTR handleButton32Interrupt();
void IRAM_ATTR handleButton19Interrupt();


// ========= INTERRUPT HANDLERS =========
void IRAM_ATTR handleButton32Interrupt() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime32 > DEBOUNCE_DELAY) {
    button32Pressed = !button32Pressed;
    button32StateChanged = true;
    lastDebounceTime32 = currentTime;
  }
}

void IRAM_ATTR handleButton19Interrupt() {
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime19 > DEBOUNCE_DELAY) {
    button19Pressed = !button19Pressed;
    button19StateChanged = true;
    lastDebounceTime19 = currentTime;
  }
}

// ========= INITIALIZATION FUNCTIONS =========
void initPins() {
  // Button pins
  pinMode(BUTTON_GND_PIN, INPUT_PULLUP);     // GPIO32: Pull-down button
  pinMode(BUTTON_VCC_PIN, INPUT_PULLDOWN);   // GPIO12: Pull-up button
  
  // LED pins
  pinMode(LED, OUTPUT);
  pinMode(LED_ON, OUTPUT);
    
  // QTR-8A sensor pins (digital)
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);
  pinMode(S6, INPUT);
  pinMode(S7, INPUT);
  pinMode(S8, INPUT);
  
  // Additional sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(SERVO_LEFT, OUTPUT);
  pinMode(SERVO_RIGHT, OUTPUT);
  
  // Set initial LED states
  digitalWrite(LED, LOW);
  digitalWrite(LED_ON, LOW);  
  Serial.println("✓ Pins initialized");
}

void initMotors() {
  Serial.println("Initializing L298N/TB6612 motor drivers...");
  
  // Setup motor pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Setup PWM channels for motor speed control
  ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, PWM_CHANNEL_LEFT);
  ledcAttachPin(ENB, PWM_CHANNEL_RIGHT);
  
  // Stop all motors initially
  stopAllMotors();
  
  Serial.println("✓ L298N/TB6612 motors initialized");
}


void setMotorSpeeds(int leftSpeed, int rightSpeed) {
  motor1Drive(leftSpeed);   // Left motor
  motor2Drive(rightSpeed);  // Right motor
}

// ========= QTR-8A DIGITAL SENSOR READING =========
void readAllSensors() {
  // Read digital values from QTR-8A sensors
  // Active-LOW: LOW (0) = line detected, HIGH (1) = no line
  sensorValues[0] = !digitalRead(S1) ? 1 : 0;   // Invert for calibration compatibility
  sensorValues[1] = !digitalRead(S2) ? 1 : 0;
  sensorValues[2] = !digitalRead(S3) ? 1 : 0;
  sensorValues[3] = !digitalRead(S4) ? 1 : 0;
  sensorValues[4] = !digitalRead(S5) ? 1 : 0;
  sensorValues[5] = !digitalRead(S6) ? 1 : 0;
  sensorValues[6] = !digitalRead(S7) ? 1 : 0;
  sensorValues[7] = !digitalRead(S8) ? 1 : 0;
}

// ========= CALIBRATION FUNCTIONS =========
void calibrateWhiteSensors() {
  Serial.println("=== CALIBRATION NỀN TRẮNG ===");
  Serial.println("Đặt robot trên nền trắng...");
  
  // BLE notification
  if (_BLEClientConnected) {
    sendResponse("INFO", "Starting white calibration in 3 seconds...");
  }
  
  // Visual countdown
  for (int countdown = 3; countdown > 0; countdown--) {
    Serial.println("Bắt đầu trong: " + String(countdown));
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    delay(800);
  }
  
  Serial.println("Đang đọc nền trắng...");
  
  // Initialize white values
  for (int i = 0; i < NUM_SENSORS; i++) {
    whiteValues[i] = 0;
  }
  
  // Take multiple samples
  for (int sample = 0; sample < CALIBRATION_SAMPLES; sample++) {
    readAllSensors();
    
    // Accumulate values
    for (int i = 0; i < NUM_SENSORS; i++) {
      whiteValues[i] += sensorValues[i];
    }
    
    // Visual feedback
    digitalWrite(LED, sample % 10 < 5);
    delay(20);
    
    // Progress indicator
    if (sample % 30 == 0) {
      int progress = (sample * 100) / CALIBRATION_SAMPLES;
      Serial.print("Progress: ");
      Serial.print(progress);
      Serial.println("%");
      
      if (_BLEClientConnected) {
        DynamicJsonDocument doc(256);
        doc["type"] = "calibration_progress";
        doc["stage"] = "white";
        doc["progress"] = progress;
        String jsonString;
        serializeJson(doc, jsonString);
        pCharacteristic->setValue(jsonString.c_str());
        pCharacteristic->notify();
      }
    }
  }
  
  // Calculate averages
  for (int i = 0; i < NUM_SENSORS; i++) {
    whiteValues[i] /= CALIBRATION_SAMPLES;
  }
  
  digitalWrite(LED, LOW);
  Serial.println("✓ Hiệu chuẩn nền trắng hoàn thành!");
}

void calibrateBlackSensors() {
  Serial.println("=== CALIBRATION ĐƯỜNG ĐEN ===");
  Serial.println("Đặt robot trên đường đen...");
  
  // BLE notification
  if (_BLEClientConnected) {
    sendResponse("INFO", "Starting black calibration in 3 seconds...");
  }
  
  // Visual countdown
  for (int countdown = 3; countdown > 0; countdown--) {
    Serial.println("Bắt đầu trong: " + String(countdown));
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    delay(800);
  }
  
  Serial.println("Đang đọc đường đen...");
  
  // Initialize black values
  for (int i = 0; i < NUM_SENSORS; i++) {
    blackValues[i] = 0;
  }
  
  // Take multiple samples
  for (int sample = 0; sample < CALIBRATION_SAMPLES; sample++) {
    readAllSensors();
    
    // Accumulate values
    for (int i = 0; i < NUM_SENSORS; i++) {
      blackValues[i] += sensorValues[i];
    }
    
    // Visual feedback
    digitalWrite(LED, sample % 10 < 5);
    delay(20);
    
    // Progress indicator
    if (sample % 30 == 0) {
      int progress = (sample * 100) / CALIBRATION_SAMPLES;
      Serial.print("Progress: ");
      Serial.print(progress);
      Serial.println("%");
      
      if (_BLEClientConnected) {
        DynamicJsonDocument doc(256);
        doc["type"] = "calibration_progress";
        doc["stage"] = "black";
        doc["progress"] = progress;
        String jsonString;
        serializeJson(doc, jsonString);
        pCharacteristic->setValue(jsonString.c_str());
        pCharacteristic->notify();
      }
    }
  }
  
  // Calculate averages
  for (int i = 0; i < NUM_SENSORS; i++) {
    blackValues[i] /= CALIBRATION_SAMPLES;
  }
  
  digitalWrite(LED, LOW);
  Serial.println("✓ Hiệu chuẩn đường đen hoàn thành!");
}

void calculateThresholds() {
  Serial.println("Đang tính toán ngưỡng...");
  
  // For digital QTR-8A sensors, thresholds are simplified to 0
  // digitizeSensors() checks if sensorValues[i] > 0 (1 = line detected, 0 = no line)
  for (int i = 0; i < NUM_SENSORS; i++) {
    thresholdValues[i] = 0;
  }
  
  isCalibrated = true;
  Serial.println("✓ Tính ngưỡng hoàn thành!");
  
  Serial.println("✓ Robot calibrated and ready to follow line!");
}

// ========= SENSOR PROCESSING =========
void digitizeSensors() {
  lineDetected = false;
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    // For digital mode, sensorValues[i] is already 0 or 1
    // Use simple threshold comparison
    digitalSensors[i] = (sensorValues[i] > 0);
    if (digitalSensors[i]) {
      lineDetected = true;
    }
  }
}

int calculateLinePosition() {
  if (!isCalibrated) return setpoint;
  
  digitizeSensors();
  
  if (!lineDetected) {
    // No line detected, return last valid position
    return lastValidPosition;
  }
  
  // Calculate weighted average position
  long weightedSum = 0;
  long sumSensors = 0;
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (digitalSensors[i]) {
      weightedSum += (long)i * 100;  // Position value (0, 100, 200, ..., 1500)
      sumSensors++;
    }
  }
  
  if (sumSensors > 0) {
    int position = weightedSum / sumSensors;
    lastValidPosition = position;
    return position;
  }
  
  return lastValidPosition;
}

// ========= PID CONTROL =========
void pidControl() {
  if (!isCalibrated) {
    stopAllMotors();
    return;
  }
  
  // Read sensors and calculate line position
  readAllSensors();
  linePosition = calculateLinePosition();
  
  // Calculate PID error
  int error = linePosition - setpoint;
  
  // Proportional term
  int proportional = error;
  
  // Integral term (with windup protection)
  integral += error;
  if (integral > 1000) integral = 1000;
  if (integral < -1000) integral = -1000;
  
  // Derivative term
  int derivative = error - lastError;
  lastError = error;
  
  // Calculate PID output
  float pidOutput = (kp * proportional) + (ki * integral) + (kd * derivative);
  
  // Constrain PID output
  int correction = constrain(pidOutput, -maxSpeed, maxSpeed);
  
  // ========= SMOOTH SPEED BOOST LOGIC =========
  // Xác định target speed
  int targetSpeed = baseSpeed;
  if (abs(error) <= straightThreshold) {
    straightLineCounter++;
    if (straightLineCounter >= boostEnableCount) {
      targetSpeed = boostSpeed;
    }
  } else {
    straightLineCounter = 0;
  }

  // Smooth transition đến target speed
  int currentBaseSpeed = previousBaseSpeed;
  speedChangeCounter++;

  if (speedChangeCounter >= speedChangeDelay) {
    speedChangeCounter = 0;
    
    if (targetSpeed > previousBaseSpeed) {
      currentBaseSpeed = min(previousBaseSpeed + speedStep, targetSpeed);
    } else if (targetSpeed < previousBaseSpeed) {
      currentBaseSpeed = max(previousBaseSpeed - speedStep, targetSpeed);
    }
    
    previousBaseSpeed = currentBaseSpeed;
  }

  // Calculate motor speeds với base speed động
  int leftSpeed = currentBaseSpeed - correction;
  int rightSpeed = currentBaseSpeed + correction;
  
  // Constrain motor speeds
  leftSpeed = constrain(leftSpeed, -maxSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, -maxSpeed, maxSpeed);
  
  // Apply speeds to motors
  setMotorSpeeds(leftSpeed, rightSpeed);
  
  // Debug output (reduced frequency)
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 1000) {
    Serial.print("Pos: ");
    Serial.print(linePosition);
    Serial.print(" | Error: ");
    Serial.print(error);
    Serial.print(" | PID: ");
    Serial.print(correction);
    Serial.print(" | Motors: L=");
    Serial.print(leftSpeed);
    Serial.print(" R=");
    Serial.println(rightSpeed);
    lastDebugTime = millis();
  }
}

void followLine() {
  if (robotRunning && isCalibrated) {
    pidControl();
    
    // Status LED blinking during operation
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 300) {
      digitalWrite(LED, !digitalRead(LED));
      lastBlink = millis();
    }
  } else {
    stopAllMotors();
    digitalWrite(LED, LOW);
    straightLineCounter = 0; // Reset counter when stopped
  }
}

// ========= BUTTON PROCESSING =========
void processButtonEvents() {
  // Skip button processing if in BLE mode
  if (currentMode == MODE_BLE) {
    // Only allow emergency stop button in BLE mode
    if (button19StateChanged && button19Pressed) {
      button19StateChanged = false;
      Serial.println("🛑 Emergency STOP Button pressed");
      robotRunning = false;
      stopAllMotors();
      robotStatus = "EMERGENCY_STOPPED";      
      if (_BLEClientConnected) {
        sendResponse("WARNING", "Emergency stop activated");
      }
    }
    return;
  }
  
  // Handle GPIO32 button press (Main Control)
  if (button32StateChanged && button32Pressed) {
    button32StateChanged = false;
    
    Serial.println("🔘 Button GPIO32 pressed");
    
    switch (currentMode) {
      case MODE_IDLE:
        currentMode = MODE_MANUAL_CALIBRATE_WHITE;
        button32PressCount = 1;
        robotRunning = false;
        stopAllMotors();
        Serial.println("➡️ Mode: Manual Calibration (White)");
        digitalWrite(LED_ON, HIGH);
        break;
        
      case MODE_MANUAL_CALIBRATE_WHITE:
        button32PressCount = 2;
        Serial.println("🟦 Starting White Calibration...");
        calibrateWhiteSensors();
        currentMode = MODE_MANUAL_CALIBRATE_BLACK;
        break;
        
      case MODE_MANUAL_CALIBRATE_BLACK:
        button32PressCount = 3;
        Serial.println("⬛ Starting Black Calibration...");
        calibrateBlackSensors();
        calculateThresholds();
        currentMode = MODE_MANUAL_RUNNING;
        break;
        
      case MODE_MANUAL_RUNNING:
        button32PressCount = 4;
        Serial.println("🚀 Starting Line Following...");
        robotRunning = true;
        robotStatus = "RUNNING";
        break;
        
      default:
        break;
    }
    
    // Visual feedback - blink LED according to press count
    for (int i = 0; i < button32PressCount; i++) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
  }
  
  // Handle GPIO19 button press (Stop Button)
  if (button19StateChanged && button19Pressed) {
    button19StateChanged = false;
    
    Serial.println("🛑 STOP Button pressed");
    
    // Stop robot immediately
    robotRunning = false;
    stopAllMotors();
    robotStatus = "STOPPED";
    
    // Reset to idle mode
    currentMode = MODE_IDLE;
    button32PressCount = 0;
    
    Serial.println("🏁 Robot stopped and reset to IDLE mode");
    
    // Visual feedback - rapid blinks
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
  }
}

// ========= DEBUG FUNCTIONS =========
void printSensorValues() {
  Serial.print("Raw: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(sensorValues[i]);
    Serial.print("\t");
  }
  
  Serial.print(" | Digital: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(digitalSensors[i] ? "1" : "0");
  }
  
  Serial.print(" | Pos: ");
  Serial.println(linePosition);
}

void printCalibrationData() {
  Serial.println("\n=== CALIBRATION DATA ===");
  Serial.println("Sensor\tWhite\tBlack\tThreshold");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(i);
    Serial.print("\t");
    Serial.print(whiteValues[i]);
    Serial.print("\t");
    Serial.print(blackValues[i]);
    Serial.print("\t");
    Serial.println(thresholdValues[i]);
  }
  Serial.println("========================\n");
}

// logic nháy led theo chế độ hiện tại
void updateModeLED() {
  switch (currentMode) {
    case MODE_IDLE:
      digitalWrite(LED_ON, LOW);
      break;
      
    case MODE_BLE:
      // Slow pulse during BLE mode
      static unsigned long lastBLEBlink = 0;
      static bool blePulseUp = true;
      static int bleIntensity = 0;
      
      if (millis() - lastBLEBlink > 20) {
        if (blePulseUp) {
          bleIntensity += 5;
          if (bleIntensity >= 255) {
            bleIntensity = 255;
            blePulseUp = false;
          }
        } else {
          bleIntensity -= 5;
          if (bleIntensity <= 0) {
            bleIntensity = 0;
            blePulseUp = true;
          }
        }
        analogWrite(LED_ON, bleIntensity);
        lastBLEBlink = millis();
      }
      break;
      
    case MODE_MANUAL_CALIBRATE_WHITE:
    case MODE_MANUAL_CALIBRATE_BLACK:
      // Slow blink during calibration mode
      static unsigned long lastCalibBlink = 0;
      if (millis() - lastCalibBlink > 500) {
        digitalWrite(LED_ON, !digitalRead(LED_ON));
        lastCalibBlink = millis();
      }
      break;
      
    case MODE_MANUAL_RUNNING:
      if (robotRunning) {
        // Fast blink during running
        static unsigned long lastRunBlink = 0;
        if (millis() - lastRunBlink > 150) {
          digitalWrite(LED_ON, !digitalRead(LED_ON));
          lastRunBlink = millis();
        }
      } else {
        digitalWrite(LED_ON, HIGH);
      }
      break;
      
    default:
      break;
  }
}
// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  delay(1000);  // Give time for serial to initialize
  
  Serial.println("🚀 Starting 16-Sensor Line Following Robot with BLE");
  Serial.println("===================================================");
  
  // Initialize hardware
  initPins();
  initMotors();
  
  // Initialize BLE
  InitBLE();
  
  // Attach button interrupts
  attachInterrupt(digitalPinToInterrupt(BUTTON_GND_PIN), handleButton32Interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_VCC_PIN), handleButton19Interrupt, RISING);
  
  Serial.println("✓ Button interrupts configured");
  Serial.println("  - GPIO32: Main control (calibration & start)");
  Serial.println("  - GPIO19: Emergency stop");
  
  // Startup light show
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_ON, HIGH);
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED_ON, LOW);
    digitalWrite(LED, LOW);
    delay(200);
  }
  
  Serial.println("\n🎯 CONTROL MODES:");
  Serial.println("1. 📱 BLE Mode: Connect via smartphone/web app");
  Serial.println("2. 🔘 Manual Mode: Use physical buttons");
  Serial.println("   - Press GPIO32 to start calibration sequence");
  Serial.println("   - Press GPIO19 anytime for EMERGENCY STOP");
  
  Serial.println("\n⚙️ Default PID Parameters:");
  Serial.println("Kp: " + String(kp));
  Serial.println("Ki: " + String(ki));
  Serial.println("Kd: " + String(kd));
  Serial.println("Base Speed: " + String(baseSpeed));
  Serial.println("Max Speed: " + String(maxSpeed));
  
  robotStatus = "READY";
  currentMode = MODE_IDLE;
  
  Serial.println("\n✅ Robot ready!");
  Serial.println("📱 Connect via BLE 'LineFollowing_Robot' or press GPIO32 to start...");
}

// ========= MAIN LOOP =========
void loop() {  
  // Process button events (manual mode only)
  processButtonEvents();
  
  // Execute line following if running
  followLine();
  
  // Update BLE status periodically
  updateBLEStatus();
  
  // Mode indicator LED
  // switch (currentMode) {
  //   case MODE_IDLE:
  //     digitalWrite(LED_ON, LOW);
  //     break;
      
  //   case MODE_BLE:
  //     // Slow pulse during BLE mode
  //     static unsigned long lastBLEBlink = 0;
  //     static bool blePulseUp = true;
  //     static int bleIntensity = 0;
      
  //     if (millis() - lastBLEBlink > 20) {
  //       if (blePulseUp) {
  //         bleIntensity += 5;
  //         if (bleIntensity >= 255) {
  //           bleIntensity = 255;
  //           blePulseUp = false;
  //         }
  //       } else {
  //         bleIntensity -= 5;
  //         if (bleIntensity <= 0) {
  //           bleIntensity = 0;
  //           blePulseUp = true;
  //         }
  //       }
  //       analogWrite(LED_ON, bleIntensity);
  //       lastBLEBlink = millis();
  //     }
  //     break;
      
  //   case MODE_MANUAL_CALIBRATE_WHITE:
  //   case MODE_MANUAL_CALIBRATE_BLACK:
  //     // Slow blink during calibration mode
  //     static unsigned long lastCalibBlink = 0;
  //     if (millis() - lastCalibBlink > 500) {
  //       digitalWrite(LED_ON, !digitalRead(LED_ON));
  //       lastCalibBlink = millis();
  //     }
  //     break;
      
  //   case MODE_MANUAL_RUNNING:
  //     if (robotRunning) {
  //       // Fast blink during running
  //       static unsigned long lastRunBlink = 0;
  //       if (millis() - lastRunBlink > 150) {
  //         digitalWrite(LED_ON, !digitalRead(LED_ON));
  //         lastRunBlink = millis();
  //       }
  //     } else {
  //       digitalWrite(LED_ON, HIGH);
  //     }
  //     break;
      
  //   default:
  //     break;
  // }
  
  // Mode indicator LED (optimized)
  static unsigned long lastLEDUpdate = 0;
  if (millis() - lastLEDUpdate > 20) {
    updateModeLED();
    lastLEDUpdate = millis();
  }
  delayMicroseconds(50);  // Small delay for stability
}
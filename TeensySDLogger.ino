// Teensy SD Card Logger for ESP32 Sensor Fusion
// Parses incoming CSV data and saves to separate files

#include <SD.h>

const int chipSelect = BUILTIN_SDCARD;

File imuFile;
File envFile;
File lightFile;

unsigned long lastBlink = 0;
bool ledState = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize Serial1 to receive from ESP32
  Serial1.begin(115200);  // RX1 (pin 0) receives from ESP32 TX
  
  // USB debug (optional)
  Serial.begin(9600);
  delay(1000);
  Serial.println("Teensy SD Logger Starting...");
  
  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    // Fast blink = SD card error
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  Serial.println("SD card initialized");
  
  // Create/initialize IMU file
  if (SD.exists("imu.csv")) {
    Serial.println("imu.csv exists, appending...");
  } else {
    imuFile = SD.open("imu.csv", FILE_WRITE);
    if (imuFile) {
      imuFile.println("teensy_ms,esp_ms,sensor,accel_x,accel_y,accel_z,magnitude");
      imuFile.close();
      Serial.println("imu.csv created");
    }
  }
  
  // Create/initialize Environmental file
  if (SD.exists("env.csv")) {
    Serial.println("env.csv exists, appending...");
  } else {
    envFile = SD.open("env.csv", FILE_WRITE);
    if (envFile) {
      envFile.println("teensy_ms,esp_ms,sensor,temperature,humidity,pressure,gas");
      envFile.close();
      Serial.println("env.csv created");
    }
  }
  
  // Create/initialize Light file
  if (SD.exists("light.csv")) {
    Serial.println("light.csv exists, appending...");
  } else {
    lightFile = SD.open("light.csv", FILE_WRITE);
    if (lightFile) {
      lightFile.println("teensy_ms,esp_ms,sensor,lux,ir,full,visible");
      lightFile.close();
      Serial.println("light.csv created");
    }
  }
  
  // 3 slow blinks = ready
  for(int i=0; i<3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
  }
  
  Serial.println("Logging started - waiting for data...");
}

void loop() {
  // Check for incoming data from ESP32
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    data.trim();  // Remove whitespace/newlines
    
    if (data.length() > 0) {
      // Quick LED pulse to show activity
      digitalWrite(LED_BUILTIN, HIGH);
      
      // Parse the CSV line
      // Format: timestamp,sensor,value1,value2,value3,value4
      int firstComma = data.indexOf(',');
      int secondComma = data.indexOf(',', firstComma + 1);
      
      if (firstComma > 0 && secondComma > 0) {
        String timestamp = data.substring(0, firstComma);
        String sensorType = data.substring(firstComma + 1, secondComma);
        
        unsigned long teensyTime = millis();
        
        // Route to appropriate file based on sensor type
        if (sensorType == "IMU" || sensorType == "IMU_ALERT") {
          imuFile = SD.open("imu.csv", FILE_WRITE);
          if (imuFile) {
            imuFile.print(teensyTime);
            imuFile.print(",");
            imuFile.println(data);
            imuFile.close();
          }
          Serial.print("IMU: ");
          Serial.println(data);
          
        } else if (sensorType == "ENV") {
          envFile = SD.open("env.csv", FILE_WRITE);
          if (envFile) {
            envFile.print(teensyTime);
            envFile.print(",");
            envFile.println(data);
            envFile.close();
          }
          Serial.print("ENV: ");
          Serial.println(data);
          
        } else if (sensorType == "LIGHT" || sensorType == "LIGHT_ALERT") {
          lightFile = SD.open("light.csv", FILE_WRITE);
          if (lightFile) {
            lightFile.print(teensyTime);
            lightFile.print(",");
            lightFile.println(data);
            lightFile.close();
          }
          Serial.print("LIGHT: ");
          Serial.println(data);
          
        } else {
          // Unknown sensor type - log to a debug file
          File debugFile = SD.open("debug.csv", FILE_WRITE);
          if (debugFile) {
            debugFile.print(teensyTime);
            debugFile.print(",");
            debugFile.println(data);
            debugFile.close();
          }
          Serial.print("UNKNOWN: ");
          Serial.println(data);
        }
      }
      
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  
  // Heartbeat blink every 2 seconds when idle
  if (millis() - lastBlink > 2000) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
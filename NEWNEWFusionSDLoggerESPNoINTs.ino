// ESP32 sensor fusion
#include <Adafruit_MPU6050.h>
#include <Adafruit_TSL2591.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

/**************************************************************************/
// Hardware
/**************************************************************************/
Adafruit_MPU6050 mpu;
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
Adafruit_AHTX0 aht;

/**************************************************************************/
// FreeRTOS Synchronization
/**************************************************************************/
SemaphoreHandle_t i2cMutex;
SemaphoreHandle_t serialMutex;

/**************************************************************************/
// IMU Task - 10Hz (every 100ms)
/**************************************************************************/
void TaskIMU(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  while(1) {
    sensors_event_t accel, gyro, temp;
    
    if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      mpu.getEvent(&accel, &gyro, &temp);
      xSemaphoreGive(i2cMutex);
      
      float magnitude = sqrt(accel.acceleration.x * accel.acceleration.x +
                            accel.acceleration.y * accel.acceleration.y +
                            accel.acceleration.z * accel.acceleration.z);
      
      if(xSemaphoreTake(serialMutex, 0) == pdTRUE) {
        Serial.printf("%lu,IMU,%.2f,%.2f,%.2f,%.2f\n",
                      millis(),
                      accel.acceleration.x,
                      accel.acceleration.y,
                      accel.acceleration.z,
                      magnitude);
        xSemaphoreGive(serialMutex);
      }
      
      // Send to Teensy
      Serial1.printf("%lu,IMU,%.2f,%.2f,%.2f,%.2f\n",
                    millis(),
                    accel.acceleration.x,
                    accel.acceleration.y,
                    accel.acceleration.z,
                    magnitude);
    }
    
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));  // 10Hz
  }
}

/**************************************************************************/
// Environmental Task - 1Hz (every 1000ms)
/**************************************************************************/
void TaskEnvironmental(void *parameter) {
  vTaskDelay(pdMS_TO_TICKS(2000));  // Let it stabilize
  
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  while(1) {
    sensors_event_t humidity, temp;
    
    if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      bool success = aht.getEvent(&humidity, &temp);
      xSemaphoreGive(i2cMutex);
      
      if(success) {
        float temperature = temp.temperature;
        float humidityVal = humidity.relative_humidity;
        
        if(xSemaphoreTake(serialMutex, 0) == pdTRUE) {
          Serial.printf("%lu,ENV,%.2f,%.2f,0.00,0.00\n",
                        millis(),
                        temperature,
                        humidityVal);
          xSemaphoreGive(serialMutex);
        }
        
        // Send to Teensy
        Serial1.printf("%lu,ENV,%.2f,%.2f,0.00,0.00\n",
                      millis(),
                      temperature,
                      humidityVal);
      }
    }
    
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));  // 1Hz
  }
}

/**************************************************************************/
// Light Sensor Task - 0.5Hz (every 2000ms)
/**************************************************************************/
void TaskLight(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  while (1) {
    if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      uint32_t lum = tsl.getFullLuminosity();
      xSemaphoreGive(i2cMutex);
      
      uint16_t ir = lum >> 16;
      uint16_t full = lum & 0xFFFF;
      uint16_t visible = full - ir;
      float lux = tsl.calculateLux(full, ir);
      
      if(xSemaphoreTake(serialMutex, 0) == pdTRUE) {
        Serial.printf("%lu,LIGHT,%.2f,%u,%u,%u\n",
                      millis(), lux, ir, full, visible);
        xSemaphoreGive(serialMutex);
      }
      
      // Send to Teensy
      Serial1.printf("%lu,LIGHT,%.2f,%u,%u,%u\n",
                    millis(), lux, ir, full, visible);
    }
    
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));  // 0.5Hz
  }
}

/**************************************************************************/
// Setup
/**************************************************************************/
void setup() {
  Serial.begin(115200);
  while(!Serial) delay(10);
  
  // Initialize Serial1 for Teensy (TX=17, RX=16)
  Serial1.begin(115200, SERIAL_8N1, 16, 17);
  delay(100);
  
  Serial.println("\n=== ESP32 Sensor Logger - CLEAN & SIMPLE ===\n");
  
  // Create mutexes
  i2cMutex = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();
  
  if(i2cMutex == NULL || serialMutex == NULL) {
    Serial.println("ERROR: Failed to create mutexes!");
    while(1);
  }
  Serial.println("✓ Mutexes created");
  
  // Initialize I2C
  Wire.begin(23, 22);
  
  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("✗ MPU6050 not found!");
    while (1) delay(10);
  }
  Serial.println("✓ MPU6050 found");
  
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  // Initialize TSL2591
  if (!tsl.begin()) {
    Serial.println("✗ TSL2591 not found!");
    while (1) delay(10);
  }
  Serial.println("✓ TSL2591 found");
  
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
  
  // Initialize AHT20
  if (!aht.begin()) {
    Serial.println("✗ AHT20 not found!");
    while (1) delay(10);
  }
  Serial.println("✓ AHT20 found");
  
  delay(1000);
  
  /**************************************************************************/
  // Create Tasks with Priorities
  /**************************************************************************/
  
  Serial.println("\n=== Creating Tasks ===");
  
  xTaskCreatePinnedToCore(TaskIMU, "IMU", 4096, NULL, 3, NULL, app_cpu);
  Serial.println("✓ IMU Task @ 10Hz");
  
  xTaskCreatePinnedToCore(TaskEnvironmental, "ENV", 4096, NULL, 2, NULL, app_cpu);
  Serial.println("✓ Environmental Task @ 1Hz");
  
  xTaskCreatePinnedToCore(TaskLight, "Light", 4096, NULL, 1, NULL, app_cpu);
  Serial.println("✓ Light Task @ 0.5Hz");
  
  Serial.println("\n=== CSV Format ===");
  Serial.println("timestamp,sensor,value1,value2,value3,value4");
  Serial.println("==================\n");
  
  delay(1000);
  Serial.println("--- LOGGING STARTED ---\n");
}

void loop() {
  // Empty - FreeRTOS handles everything
}
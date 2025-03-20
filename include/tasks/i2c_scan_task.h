#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>

// Configuration
#define SENSOR_I2C_ADDRESS 0x6B
#define SENSOR_READY_CHECK_INTERVAL 100  // ms
#define SENSOR_READY_TIMEOUT 1000        // ms
#define I2C_SDA_PIN 18
#define I2C_SCL_PIN 17

// Sensor data structure
struct SensorData {
    // Particle mass concentrations
    float pm1p0;
    float pm2p5;
    float pm4p0;
    float pm10p0;
    
    // Environmental parameters
    float humidity;
    float temperature;
    
    // Air quality indices
    float vocIndex;
    float noxIndex;
    
    // Gas concentration
    float co2;
    
    // Raw values
    int16_t rawHumidity;    // Scaled with factor 100: RH [%] = value / 100
    int16_t rawTemperature; // Scaled with factor 200: T [°C] = value / 200
    uint16_t rawVOC;       // Raw VOC ticks without scale factor
    uint16_t rawNOx;       // Raw NOx ticks without scale factor
    uint16_t rawCO2;       // Not interpolated CO₂ concentration [ppm]
};

// Task function declaration
void i2cScanTask(void* parameter);

// Task handle declaration
extern TaskHandle_t xI2CScanTaskHandle; 
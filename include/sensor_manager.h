#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>
#include <FreeRTOS.h>
#include <task.h>

struct SensorData {
    // Particle mass concentrations
    float pm1p0;
    float pm2p5;
    float pm4p0;
    float pm10p0;
    
    // Environmental parameters
    float temperature;
    float humidity;
    
    // Air quality indices
    float vocIndex;
    float noxIndex;
    
    // Raw values
    int16_t rawHumidity;    // Scaled with factor 100: RH [%] = value / 100
    int16_t rawTemperature; // Scaled with factor 200: T [°C] = value / 200
    uint16_t rawVOC;       // Raw VOC ticks without scale factor
    uint16_t rawNOx;       // Raw NOx ticks without scale factor
    uint16_t rawCO2;       // Not interpolated CO₂ concentration [ppm]
    
    // Gas concentration
    uint16_t co2;  // Changed from float to uint16_t for ppm
};

class SensorManager {
public:
    SensorManager();
    bool begin();
    bool update();
    const SensorData& getData() const { return _data; }
    
    // Made public for I2C task
    SensirionI2cSen66 _sensor;
    SensorData _data;
    uint32_t _lastUpdate;
    static const uint32_t UPDATE_INTERVAL = 1000; // 1 second
    
    static constexpr uint8_t SEN66_I2C_ADDR = 0x6B;
    static constexpr uint8_t I2C_SDA_PIN = 18;  // Adjust according to your wiring
    static constexpr uint8_t I2C_SCL_PIN = 17;  // Updated to match your hardware
}; 
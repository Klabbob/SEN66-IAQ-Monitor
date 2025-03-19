#include "sensor_manager.h"

SensorManager::SensorManager() : _lastUpdate(0) {
    // Initialize data structure
    memset(&_data, 0, sizeof(_data));
}

bool SensorManager::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); // Set I2C clock to 100kHz for better reliability
    
    uint16_t error;
    char errorMessage[256];
    
    _sensor.begin(Wire, SEN66_I2C_ADDR);
    
    error = _sensor.deviceReset();
    if (error) {
        #ifdef DEBUG_MODE
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return false;
    }
    
    // Start measurement
    error = _sensor.startContinuousMeasurement();
    if (error) {
        #ifdef DEBUG_MODE
        Serial.println("Error executing startContinuousMeasurement");
        #endif
        return false;
    }
    
    return true;
}

bool SensorManager::update() {
    // This function is now deprecated as updates are handled by the task
    return true;
} 
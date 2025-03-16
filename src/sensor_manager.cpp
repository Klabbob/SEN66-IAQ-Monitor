#include "sensor_manager.h"

SensorManager::SensorManager() : _isInitialized(false) {
    memset(&_data, 0, sizeof(SensorData));
}

bool SensorManager::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); // Set I2C clock to 100kHz for better reliability
    
    _sensor.begin(Wire, SEN66_I2C_ADDR);
    
    uint16_t error;
    char errorMessage[256];
    
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
    
    _isInitialized = true;
    return true;
}

bool SensorManager::update() {
    if (!_isInitialized) return false;
    
    uint16_t error;
    float tempPm1p0, tempPm2p5, tempPm4p0, tempPm10p0;
    float tempHumidity, tempTemperature;
    float tempVocIndex, tempNoxIndex;
    uint16_t tempCo2;
    int16_t tempRawHumidity, tempRawTemperature;
    uint16_t tempRawVOC, tempRawNOx, tempRawCO2;
    
    error = _sensor.readMeasuredValues(
        tempPm1p0, tempPm2p5, tempPm4p0, tempPm10p0,
        tempHumidity, tempTemperature,
        tempVocIndex, tempNoxIndex,
        tempCo2
    );
    
    if (error) {
        #ifdef DEBUG_MODE
        char errorMessage[256];
        errorToString(error, errorMessage, 256);
        Serial.print("Error reading values: ");
        Serial.println(errorMessage);
        #endif
        return false;
    }
    
    // Read raw values
    error = _sensor.readMeasuredRawValues(
        tempRawHumidity, tempRawTemperature,
        tempRawVOC, tempRawNOx, tempRawCO2
    );
    if (error) {
        #ifdef DEBUG_MODE
        char errorMessage[256];
        errorToString(error, errorMessage, 256);
        Serial.print("Error reading raw values: ");
        Serial.println(errorMessage);
        #endif
        return false;
    }
    
    // Update particle mass concentrations
    _data.pm1p0 = tempPm1p0;
    _data.pm2p5 = tempPm2p5;
    _data.pm4p0 = tempPm4p0;
    _data.pm10p0 = tempPm10p0;
    
    // Update environmental parameters
    _data.humidity = tempHumidity;
    _data.temperature = tempTemperature;
    
    // Update air quality indices
    _data.vocIndex = tempVocIndex;
    _data.noxIndex = tempNoxIndex;
    
    // Update raw values
    _data.rawHumidity = tempRawHumidity;
    _data.rawTemperature = tempRawTemperature;
    _data.rawVOC = tempRawVOC;
    _data.rawNOx = tempRawNOx;
    _data.rawCO2 = tempRawCO2;
    
    // Update gas concentration
    _data.co2 = tempCo2;
    
    return true;
} 
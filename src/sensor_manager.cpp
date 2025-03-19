#include "sensor_manager.h"
#include "task_hierarchy.h"
#include "task_utils.h"

// Static member initialization
TaskHandle_t SensorManager::xI2CScanTaskHandle = nullptr;

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
    
    // Launch I2C scan task
    if (launchTaskWithVerification(
        i2cScanTask,
        I2C_SCAN_TASK_NAME,
        I2C_STACK_SIZE,
        this,
        TIER_II_PRIORITY,
        &xI2CScanTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create I2C scan task");
        return false;
    }
    
    return true;
}

void SensorManager::i2cScanTask(void* parameter) {
    SensorManager* manager = static_cast<SensorManager*>(parameter);
    
    while (true) {
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (currentTime - manager->_lastUpdate >= UPDATE_INTERVAL) {
            uint16_t error;
            float tempPm1p0, tempPm2p5, tempPm4p0, tempPm10p0;
            float tempHumidity, tempTemperature;
            float tempVocIndex, tempNoxIndex;
            uint16_t tempCo2;
            int16_t tempRawHumidity, tempRawTemperature;
            uint16_t tempRawVOC, tempRawNOx, tempRawCO2;
            
            error = manager->_sensor.readMeasuredValues(
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
            } else {
                // Read raw values
                error = manager->_sensor.readMeasuredRawValues(
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
                } else {
                    // Update particle mass concentrations
                    manager->_data.pm1p0 = tempPm1p0;
                    manager->_data.pm2p5 = tempPm2p5;
                    manager->_data.pm4p0 = tempPm4p0;
                    manager->_data.pm10p0 = tempPm10p0;
                    
                    // Update environmental parameters
                    manager->_data.humidity = tempHumidity;
                    manager->_data.temperature = tempTemperature;
                    
                    // Update air quality indices
                    manager->_data.vocIndex = tempVocIndex;
                    manager->_data.noxIndex = tempNoxIndex;
                    
                    // Update raw values
                    manager->_data.rawHumidity = tempRawHumidity;
                    manager->_data.rawTemperature = tempRawTemperature;
                    manager->_data.rawVOC = tempRawVOC;
                    manager->_data.rawNOx = tempRawNOx;
                    manager->_data.rawCO2 = tempRawCO2;
                    
                    // Update gas concentration
                    manager->_data.co2 = tempCo2;
                    
                    manager->_lastUpdate = currentTime;
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  // Small delay to prevent task starvation
    }
}

bool SensorManager::update() {
    // This function is now deprecated as updates are handled by the task
    return true;
} 
#include "tasks/i2c_scan_task.h"
#include "tasks/live_data_manager.h"

// Initialize sensor
bool initSensor(SensirionI2cSen66& sensor) {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); // Set I2C clock to 100kHz for better reliability
    
    uint16_t error;
    char errorMessage[256];
    
    sensor.begin(Wire, SENSOR_I2C_ADDRESS);
    
    error = sensor.deviceReset();
    if (error) {
        #ifdef DEBUG_MODE
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return false;
    }
    
    // Start measurement
    error = sensor.startContinuousMeasurement();
    if (error) {
        #ifdef DEBUG_MODE
        Serial.println("Error executing startContinuousMeasurement");
        #endif
        return false;
    }
    
    return true;
}

void i2cScanTask(void* parameter) {
    // Initialize sensor
    SensirionI2cSen66 sensor;
    if (!initSensor(sensor)) {
        Serial.println("Failed to initialize sensor!");
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize data structure
    SensorData data = {
        .pm1p0 = 0.0f,
        .pm2p5 = 0.0f,
        .pm4p0 = 0.0f,
        .pm10p0 = 0.0f,
        .humidity = 0.0f,
        .temperature = 0.0f,
        .vocIndex = 0.0f,
        .noxIndex = 0.0f,
        .co2 = 0.0f,
        .rawHumidity = 0,
        .rawTemperature = 0,
        .rawVOC = 0,
        .rawNOx = 0,
        .rawCO2 = 0
    };
    
    uint32_t lastUpdate = 0;
    
    while (true) {
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Wait 900ms before checking if data is ready
        if (currentTime - lastUpdate >= 900) {
            uint8_t padding;
            bool dataReady;
            uint16_t error = sensor.getDataReady(padding, dataReady);
            
            if (!error && dataReady) {
                float tempPm1p0, tempPm2p5, tempPm4p0, tempPm10p0;
                float tempHumidity, tempTemperature;
                float tempVocIndex, tempNoxIndex;
                uint16_t tempCo2;
                int16_t tempRawHumidity, tempRawTemperature;
                uint16_t tempRawVOC, tempRawNOx, tempRawCO2;
                
                error = sensor.readMeasuredValues(
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
                    error = sensor.readMeasuredRawValues(
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
                        data.pm1p0 = tempPm1p0;
                        data.pm2p5 = tempPm2p5;
                        data.pm4p0 = tempPm4p0;
                        data.pm10p0 = tempPm10p0;
                        
                        // Update environmental parameters
                        data.humidity = tempHumidity;
                        data.temperature = tempTemperature;
                        
                        // Update air quality indices
                        data.vocIndex = tempVocIndex;
                        data.noxIndex = tempNoxIndex;
                        
                        // Update raw values
                        data.rawHumidity = tempRawHumidity;
                        data.rawTemperature = tempRawTemperature;
                        data.rawVOC = tempRawVOC;
                        data.rawNOx = tempRawNOx;
                        data.rawCO2 = tempRawCO2;
                        
                        // Update gas concentration
                        data.co2 = tempCo2;
                        
                        lastUpdate = currentTime;
                        
                        // Publish data through LiveDataManager
                        LiveDataManager::getInstance().publish(data);
                    }
                }
            } else {
                // Data not ready, wait another 100ms
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        } else {
            // Wait until we're close to the next reading time
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
} 
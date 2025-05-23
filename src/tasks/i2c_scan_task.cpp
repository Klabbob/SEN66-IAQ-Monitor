#include "tasks/i2c_scan_task.h"
#include "tasks/live_data_manager.h"
#include <nvs_flash.h>
#include <esp_partition.h>
#include <esp_err.h>

// Initialize static member variables
int32_t I2CScanTask::currentAltitude = 0;
bool I2CScanTask::applyAltitude = false;
int32_t I2CScanTask::currentFRCValue = 0;
bool I2CScanTask::performFRC = false;
uint16_t I2CScanTask::ucorrection = 0;
int16_t I2CScanTask::correction = -32768;

// I2CScanTask method implementations
void I2CScanTask::setAltitude(int32_t altitude) {
    currentAltitude = altitude;
    applyAltitude = true;
    
    // Save altitude to preferences
    Preferences prefs;
    if (!prefs.begin(PREF_NAMESPACE, false)) {
        #ifdef DEBUG_MODE
        Serial.println("Failed to open preferences for writing");
        #endif
        return;
    }
    prefs.putInt(PREF_ALTITUDE_KEY, altitude);
    prefs.end();
    
    #ifdef DEBUG_MODE
    Serial.printf("Altitude set to: %d meters\n", currentAltitude);
    #endif
}

int16_t I2CScanTask::setFRCValue(int32_t frcValue, uint32_t timeout) {
    I2CScanTask::correction = -32768;
    currentFRCValue = frcValue;
    performFRC = true;
    #ifdef DEBUG_MODE
    Serial.printf("FRC value set to: %d ppm\n", currentFRCValue);
    #endif
    // save current time and loop until timeout
    uint32_t startTime = millis();
    while (millis() - startTime < timeout) {
        if (I2CScanTask::correction != -32768) {
            #ifdef DEBUG_MODE
            Serial.printf("FRC calibration successful. Correction value: %d ppm\n", I2CScanTask::correction);
            #endif
            return I2CScanTask::correction;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    #ifdef DEBUG_MODE
    Serial.printf("FRC calibration timed out\n");
    #endif
    return I2CScanTask::correction;
}

// Initialize sensor
bool I2CScanTask::initSensor(SensirionI2cSen66& sensor) {
    // Initialize I2C
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    Wire.setClock(100000); // Set I2C clock to 100kHz
    
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

    // Set temperature acceleration parameters
    error = sensor.setTemperatureAccelerationParameters(
        static_cast<uint16_t>(K),
        static_cast<uint16_t>(P),
        static_cast<uint16_t>(T1),
        static_cast<uint16_t>(T2)
    );
    if (error) {
        #ifdef DEBUG_MODE
        Serial.print("Error setting temperature acceleration parameters: ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return false;
    }

    // Set temperature offset parameters
    error = sensor.setTemperatureOffsetParameters(
        static_cast<int16_t>(SLOT_0_OFFSET),
        static_cast<int16_t>(SLOT_0_SLOPE),
        static_cast<uint16_t>(SLOT_0_TIME_CONSTANT),
        static_cast<uint16_t>(SLOT_0_SLOT_TIME)
    );
    if (error) {
        #ifdef DEBUG_MODE
        Serial.print("Error setting temperature offset parameters: ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return false;
    }
    
    // Read altitude from preferences
    Preferences prefs;
    if (!prefs.begin(PREF_NAMESPACE, true)) {
        #ifdef DEBUG_MODE
        Serial.println("Failed to open preferences for reading, using default altitude (0)");
        #endif
        currentAltitude = 0;  // Use default value
    } else {
        currentAltitude = prefs.getInt(PREF_ALTITUDE_KEY, 0);  // Default to 0 if not set
        prefs.end();
    }
    
    // Apply altitude if it's not 0
    if (currentAltitude != 0) {
        error = sensor.setSensorAltitude(static_cast<uint16_t>(currentAltitude));
        if (error) {
            #ifdef DEBUG_MODE
            Serial.print("Error setting altitude: ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
            #endif
            return false;
        }
        #ifdef DEBUG_MODE
        Serial.printf("Applied stored altitude: %d meters\n", currentAltitude);
        #endif
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

void I2CScanTask::i2cScanTask(void* parameter) {
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
        .rawCO2 = 0,
        .runtime_ticks = 0
    };
    
    uint32_t lastUpdate = 0;
    
    while (true) {
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // if a new altitude is set, stop the measurement, set the altitude, and start the measurement again
        if (applyAltitude) {
            // Stop continuous measurement
            uint16_t error = sensor.stopMeasurement();
            if (error) {
                #ifdef DEBUG_MODE
                char errorMessage[256];
                errorToString(error, errorMessage, 256);
                Serial.print("Error stopping measurement: ");
                Serial.println(errorMessage);
                #endif
            }
            
            // Set the altitude using the correct method
            error = sensor.setSensorAltitude(static_cast<uint16_t>(currentAltitude));
            if (error) {
                #ifdef DEBUG_MODE
                char errorMessage[256];
                errorToString(error, errorMessage, 256);
                Serial.print("Error setting altitude: ");
                Serial.println(errorMessage);
                #endif
            }

            // Start measurement
            error = sensor.startContinuousMeasurement();
            if (error) {
                #ifdef DEBUG_MODE
                Serial.println("Error executing startContinuousMeasurement");
                #endif
            }
            
            data.runtime_ticks = 0;
            applyAltitude = false;
        }

        // if a new FRC value is set, stop the measurement, set the FRC value, and start the measurement again
        if (performFRC) {
            // Stop continuous measurement
            uint16_t error = sensor.stopMeasurement();
            if (error) {
                #ifdef DEBUG_MODE
                Serial.println("Error executing stopMeasurement");
                #endif
            }
            
            // Set the FRC value
            error = sensor.performForcedCo2Recalibration(static_cast<uint16_t>(currentFRCValue), I2CScanTask::ucorrection);
            // Adjust correction value: FRC correction [ppm CO2] = return value - 0x8000
            I2CScanTask::correction = I2CScanTask::ucorrection - 0x8000;
            if (error) {
                #ifdef DEBUG_MODE
                Serial.println("Error setting FRC value");
                #endif
            } else {
                #ifdef DEBUG_MODE
                Serial.printf("FRC calibration successful. Correction value: %d ppm CO2\n", I2CScanTask::correction);
                #endif
            }   
            
            // Start measurement
            error = sensor.startContinuousMeasurement();
            if (error) {
                #ifdef DEBUG_MODE
                Serial.println("Error executing startContinuousMeasurement");
                #endif
            }
            
            data.runtime_ticks = 0;
            performFRC = false;
        }
        
        // Wait 1000ms before checking if data is ready
        if (currentTime - lastUpdate >= SENSOR_READY_CHECK_INTERVAL) {
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
                        
                        // Increment runtime counter
                        data.runtime_ticks++;
                        
                        lastUpdate = currentTime;
                        
                        // Publish data through LiveDataManager
                        LiveDataManager::getInstance().publish(data);
                    }
                }
            } else {
                // Data not ready, wait another 100ms
                vTaskDelay(pdMS_TO_TICKS(SENSOR_READY_RECHECK_TIME));
            }
        } else {
            // Wait until we're close to the next reading time
            vTaskDelay(pdMS_TO_TICKS(SENSOR_READY_RECHECK_TIME));
        }
    }
} 
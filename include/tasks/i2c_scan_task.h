#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>
#include <FreeRTOS.h>
#include <task.h>
#include <Preferences.h>
#include "definitions.h"

class I2CScanTask {
public:
    static I2CScanTask& getInstance() {
        static I2CScanTask instance;
        return instance;
    }
    /**
     * @brief Sets the current altitude for sensor compensation
     * @param altitude Altitude in meters above sea level
     * @details Updates the stored altitude value used for environmental compensation
     *          in sensor measurements. Altitude affects air pressure and density.
     *          The value is persisted in non-volatile storage.
     */
    void setAltitude(int32_t altitude);

    /**
     * @brief Sets the Fast Response Curve (FRC) value for CO2 calibration
     * @param frcValue Target CO2 concentration in parts per million (ppm)
     * @param timeout Maximum time in milliseconds to wait for calibration
     * @return The calibration correction value in ppm, or original frcValue if timeout occurs
     * @details Performs forced recalibration of the CO2 sensor using a reference gas concentration.
     *          The correction value indicates the sensor's offset from the reference.
     *          A positive correction means the sensor was reading too low.
     *          A negative correction means the sensor was reading too high.
     */
    int16_t setFRCValue(int32_t frcValue, uint32_t timeout = 3000);

    // Static task function
    static void i2cScanTask(void* parameter);

private:
    I2CScanTask() = default;
    ~I2CScanTask() = default;
    I2CScanTask(const I2CScanTask&) = delete;
    I2CScanTask& operator=(const I2CScanTask&) = delete;

    static int32_t currentAltitude;  // Altitude in meters
    static bool applyAltitude;
    static int32_t currentFRCValue;  // FRC value in ppm
    static bool performFRC;
    static uint16_t ucorrection;    // unsinged Correction value for FRC calibration
    static int16_t correction;      // Correction value for FRC calibration
    
    // Static sensor initialization function
    static bool initSensor(SensirionI2cSen66& sensor);
};

// Task handle declaration
extern TaskHandle_t xI2CScanTaskHandle; 
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>
#include <FreeRTOS.h>
#include <task.h>
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
     */
    void setAltitude(int32_t altitude);

    /**
     * @brief Sets the Fast Response Curve (FRC) value for VOC/NOx compensation
     * @param frcValue FRC value in parts per million (ppm)
     * @return The set FRC value
     * @details Updates the stored FRC value used for VOC and NOx index calibration.
     *          FRC helps improve accuracy of gas measurements by providing a reference point.
     */
    int32_t setFRCValue(int32_t frcValue);

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
    static uint16_t correction;      // Correction value for FRC calibration
    // Static sensor initialization function
    static bool initSensor(SensirionI2cSen66& sensor);
};

// Task handle declaration
extern TaskHandle_t xI2CScanTaskHandle; 
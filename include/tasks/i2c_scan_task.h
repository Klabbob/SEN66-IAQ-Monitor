#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>
#include <FreeRTOS.h>
#include <task.h>
#include "definitions.h"

// Local variables for altitude and FRC
static int32_t currentAltitude = 0;  // Altitude in meters
static int32_t currentFRCValue = 0;  // FRC value in ppm

// Task function declaration
void i2cScanTask(void* parameter);
bool initSensor(SensirionI2cSen66& sensor);
void setAltitude(int32_t altitude);
int32_t setFRCValue(int32_t frcValue);

// Task handle declaration
extern TaskHandle_t xI2CScanTaskHandle; 
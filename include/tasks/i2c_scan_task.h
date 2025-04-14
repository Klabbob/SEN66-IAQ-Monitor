#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>
#include <FreeRTOS.h>
#include <task.h>
#include "definitions.h"

// Configuration
#define SENSOR_I2C_ADDRESS 0x6B
#define SENSOR_READY_CHECK_INTERVAL 100  // ms
#define SENSOR_READY_TIMEOUT 1000        // ms
#define I2C_SDA_PIN 18
#define I2C_SCL_PIN 17

// Task function declaration
void i2cScanTask(void* parameter);
bool initSensor(SensirionI2cSen66& sensor);

// Task handle declaration
extern TaskHandle_t xI2CScanTaskHandle; 
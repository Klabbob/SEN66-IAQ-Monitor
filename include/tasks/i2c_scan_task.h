#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include "sensor_manager.h"

// Configuration
#define UPDATE_INTERVAL 1000  // Update interval in milliseconds (1 second)

// Task function declaration
void i2cScanTask(void* parameter);

// Task handle declaration
extern TaskHandle_t xI2CScanTaskHandle; 
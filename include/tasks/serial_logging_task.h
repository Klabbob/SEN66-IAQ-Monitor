#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include "sensor_manager.h"

// External declaration of the global sensor manager
extern SensorManager sensorManager;

// Task function declaration
void serialLoggingTask(void* parameter);

// Task handle declaration
extern TaskHandle_t xSerialLogTaskHandle; 
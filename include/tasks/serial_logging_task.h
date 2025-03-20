#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include "tasks/i2c_scan_task.h"

// Task function declaration
void serialLoggingTask(void* parameter);

// Task handle declaration
extern TaskHandle_t xSerialLogTaskHandle; 
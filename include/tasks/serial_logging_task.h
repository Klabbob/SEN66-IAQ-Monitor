#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include "definitions.h"

// Task function declaration
void serialLoggingTask(void* parameter);

// Task handle declaration
extern TaskHandle_t xSerialLogTaskHandle; 
#pragma once

#include <FreeRTOS.h>
#include <task.h>

// Task Priority Tiers
#define TIER_I_PRIORITY    3   // User interaction tasks (highest)
#define TIER_II_PRIORITY   2   // Sensor data fetching tasks
#define TIER_III_PRIORITY  1   // Housekeeping tasks (lowest)

// Task Stack Sizes (in bytes)
#define DEFAULT_STACK_SIZE     (10 * 1024)  // 10KB for most tasks
#define I2C_STACK_SIZE         (20 * 1024)  // 20KB for I2C tasks

// Task Names
#define I2C_SCAN_TASK_NAME     "I2CScanTask"
#define SERIAL_LOG_TASK_NAME   "SerialLogTask"

// Task Handles
extern TaskHandle_t xI2CScanTaskHandle;
extern TaskHandle_t xSerialLogTaskHandle; 
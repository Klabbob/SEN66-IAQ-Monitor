#include "task_hierarchy.h"
#include "tasks/task_utils.h"
#include "tasks/serial_logging_task.h"
#include "tasks/i2c_scan_task.h"
#include "tasks/live_data_manager.h"

// Global variables
TaskHandle_t xSerialLogTaskHandle = nullptr;
TaskHandle_t xI2CScanTaskHandle = nullptr;

void setup() {
    Serial.begin(115200);
    
    // Wait for serial to be ready
    while (!Serial) delay(100);
    Serial.println("SEN66 Sensor Test Starting...");
    
    // Launch LiveDataManager task first
    if (launchTaskWithVerification(
        LiveDataManager::liveDataManagerTask,
        "LiveDataManager",
        DEFAULT_STACK_SIZE,
        nullptr,
        TIER_I_PRIORITY,
        &LiveDataManager::xLiveDataManagerTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create LiveDataManager task");
        while (1) delay(100);
    }
    
    // Launch I2C scan task
    if (launchTaskWithVerification(
        i2cScanTask,
        I2C_SCAN_TASK_NAME,
        I2C_STACK_SIZE,
        nullptr,
        TIER_II_PRIORITY,
        &xI2CScanTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create I2C scan task");
        while (1) delay(100);
    }
    
    // Launch serial logging task
    if (launchTaskWithVerification(
        serialLoggingTask,
        SERIAL_LOG_TASK_NAME,
        DEFAULT_STACK_SIZE,
        nullptr,
        TIER_III_PRIORITY,
        &xSerialLogTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create serial logging task");
        while (1) delay(100);
    }
    
    Serial.println("Tasks initialized successfully!");
    Serial.println("Starting measurements...");
    Serial.println("----------------------------------------");
}

void loop() {
    // Do nothing, all operations being taken care of by tasks
    vTaskSuspend(NULL);  // Suspend the main loop task
} 
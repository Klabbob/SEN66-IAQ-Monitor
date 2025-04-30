#include "Arduino.h"
#include <WiFi.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "task_hierarchy.h"
#include "tasks/task_utils.h"
#include "tasks/serial_logging_task.h"
#include "tasks/i2c_scan_task.h"
#include "tasks/live_data_manager.h"
#include "tasks/display_task.h"
#include "tasks/button_handler.h"
#include "definitions.h"

// Global variables
TaskHandle_t xSerialLogTaskHandle = nullptr;
TaskHandle_t xI2CScanTaskHandle = nullptr;

void disableWireless() {
    // Disable WiFi
    WiFi.disconnect(true);  // Disconnect and delete credentials
    WiFi.mode(WIFI_OFF);    // Set WiFi to off mode
    
    // Disable Bluetooth
    btStop();              // Stop Bluetooth
    esp_bt_controller_disable();  // Disable BT controller
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);  // Release BT memory
}

void setup() {
    Serial.begin(115200);
    
    // // Wait for serial to be ready
    // while (!Serial) delay(100);
    // Serial.println("SEN66 Sensor Test Starting...");

      // Set CPU frequency to 80MHz
    setCpuFrequencyMhz(80);
    
    // Disable wireless features
    disableWireless();

    // Enable power from 5v Pin or Battery 
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, 1);
    
    // Launch LiveDataManager task first
    if (launchTaskWithVerification(
        LiveDataManager::liveDataManagerTask,
        LIVE_DATA_TASK_NAME,
        LIVE_DATA_STACK_SIZE,
        nullptr,
        TIER_I_PRIORITY,
        &LiveDataManager::xLiveDataManagerTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create LiveDataManager task");
        while (1) delay(100);
    }
    
    // Launch I2C scan task
    if (launchTaskWithVerification(
        I2CScanTask::i2cScanTask,
        I2C_SCAN_TASK_NAME,
        I2C_STACK_SIZE,
        nullptr,
        TIER_II_PRIORITY,
        &xI2CScanTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create I2C scan task");
        while (1) delay(100);
    }
    
    #ifdef SERIAL_LOGGING
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
    #endif
    // Launch display task
    if (launchTaskWithVerification(
        DisplayTask::displayTask,
        DISPLAY_TASK_NAME,
        DISPLAY_STACK_SIZE,
        nullptr,
        TIER_I_PRIORITY,
        &DisplayTask::xDisplayTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create display task");
        while (1) delay(100);
    }

    // Launch button task
    if (launchTaskWithVerification(
        ButtonHandler::buttonTask,
        "ButtonTask",
        DEFAULT_STACK_SIZE,
        nullptr,
        TIER_II_PRIORITY,
        &ButtonHandler::xButtonTaskHandle
    ) != pdPASS) {
        Serial.println("Failed to create button task");
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
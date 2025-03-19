#include "task_utils.h"
#include <Arduino.h>

BaseType_t launchTaskWithVerification(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    uint32_t usStackDepth,
    void* pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* pxCreatedTask
) {
    // Verify task name length
    if (strlen(pcName) > configMAX_TASK_NAME_LEN) {
        Serial.println("Error: Task name too long");
        return pdFAIL;
    }

    // Create the task
    BaseType_t xReturned = xTaskCreate(
        pvTaskCode,
        pcName,
        usStackDepth,
        pvParameters,
        uxPriority,
        pxCreatedTask
    );

    // Verify task creation
    if (xReturned != pdPASS) {
        Serial.println("Error: Failed to create task");
        return pdFAIL;
    }

    return pdPASS;
} 
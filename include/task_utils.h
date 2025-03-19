#pragma once

#include <FreeRTOS.h>
#include <task.h>

/**
 * @brief Launches a FreeRTOS task with verification
 * 
 * @param pvTaskCode Task function pointer
 * @param pcName Task name
 * @param usStackDepth Stack size in bytes
 * @param pvParameters Task parameters
 * @param uxPriority Task priority
 * @param pxCreatedTask Task handle pointer
 * @return BaseType_t pdPASS if successful, pdFAIL otherwise
 */
BaseType_t launchTaskWithVerification(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    uint32_t usStackDepth,
    void* pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* pxCreatedTask
); 
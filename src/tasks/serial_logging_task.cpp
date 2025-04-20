#include <Arduino.h>
#include "tasks/serial_logging_task.h"
#include "tasks/live_data_manager.h"

// Queue handle for receiving data
static QueueHandle_t xDataQueue = nullptr;

void serialLoggingTask(void* parameter) {
    // Subscribe to LiveDataManager
    if (!LiveDataManager::getInstance().subscribe(xTaskGetCurrentTaskHandle(), &xDataQueue)) {
        Serial.println("Failed to subscribe to LiveDataManager!");
        vTaskDelete(NULL);
        return;
    }
    
    QueueMessage message;
    while (true) {
        // Wait for new data
        if (xQueueReceive(xDataQueue, &message, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) == pdTRUE) {
            const SensorData& data = message.data;
            
            // Print labels
            Serial.println("PM1.0\tPM2.5\tPM4.0\tPM10.0\tRH\tT\tVOC\tNOx\tCO2\tRaw RH\tRaw T\tRaw VOC\tRaw NOx\tRaw CO2");
            
            // Print all values in one line
            Serial.print(data.pm1p0); Serial.print("\t");
            Serial.print(data.pm2p5); Serial.print("\t");
            Serial.print(data.pm4p0); Serial.print("\t");
            Serial.print(data.pm10p0); Serial.print("\t");
            Serial.print(data.humidity); Serial.print("\t");
            Serial.print(data.temperature); Serial.print("\t");
            Serial.print((int)data.vocIndex); Serial.print("\t");
            Serial.print((int)data.noxIndex); Serial.print("\t");
            Serial.print((int)data.co2); Serial.print("\t");
            Serial.print(data.rawHumidity / 100.0f); Serial.print("\t");
            Serial.print(data.rawTemperature / 200.0f); Serial.print("\t");
            Serial.print((int)data.rawVOC); Serial.print("\t");
            Serial.print((int)data.rawNOx); Serial.print("\t");
            Serial.println((int)data.rawCO2);
        }
        
        // Small delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(10));
    }
} 
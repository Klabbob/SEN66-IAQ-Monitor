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
            
            // Print processed sensor readings
            Serial.println("Processed Sensor Readings:");
            Serial.println("----------------------------------------");
            Serial.println("Particle Mass Concentrations:");
            Serial.print("  PM1.0: "); Serial.print(data.pm1p0); Serial.println(" μg/m³");
            Serial.print("  PM2.5: "); Serial.print(data.pm2p5); Serial.println(" μg/m³");
            Serial.print("  PM4.0: "); Serial.print(data.pm4p0); Serial.println(" μg/m³");
            Serial.print("  PM10.0: "); Serial.print(data.pm10p0); Serial.println(" μg/m³");
            
            Serial.println("\nEnvironmental Parameters:");
            Serial.print("  Temperature: "); Serial.print(data.temperature); Serial.println(" °C");
            Serial.print("  Humidity: "); Serial.print(data.humidity); Serial.println(" %RH");
            
            Serial.println("\nAir Quality Indices:");
            Serial.print("  VOC Index: "); Serial.print(data.vocIndex); Serial.println(" (0-500)");
            Serial.print("  NOx Index: "); Serial.print(data.noxIndex); Serial.println(" (0-500)");
            
            Serial.println("\nGas Concentration:");
            Serial.print("  CO2: "); Serial.print(data.co2); Serial.println(" ppm");
            
            // Print raw values
            Serial.println("\nRaw Sensor Values:");
            Serial.println("----------------------------------------");
            Serial.println("Environmental Parameters (Raw):");
            Serial.print("  Humidity: "); Serial.print(data.rawHumidity); Serial.print(" (raw) = ");
            Serial.print(data.rawHumidity / 100.0f); Serial.println(" %RH");
            Serial.print("  Temperature: "); Serial.print(data.rawTemperature); Serial.print(" (raw) = ");
            Serial.print(data.rawTemperature / 200.0f); Serial.println(" °C");
            
            Serial.println("\nGas Sensors (Raw):");
            Serial.print("  VOC: "); Serial.print(data.rawVOC); Serial.println(" (raw ticks)");
            Serial.print("  NOx: "); Serial.print(data.rawNOx); Serial.println(" (raw ticks)");
            Serial.print("  CO2: "); Serial.print(data.rawCO2); Serial.println(" (not interpolated [ppm])");
            Serial.println("----------------------------------------\n");
        }
        
        // Small delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(10));
    }
} 
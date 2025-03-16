#include "sensor_manager.h"

SensorManager sensorManager;

void setup() {
    Serial.begin(115200);
    
    // Wait for serial to be ready
    while (!Serial) delay(100);
    Serial.println("SEN66 Sensor Test Starting...");

    if (!sensorManager.begin()) {
        Serial.println("Failed to initialize sensor!");
        while (1) delay(100);
    }
    
    Serial.println("Sensor initialized successfully!");
    Serial.println("Starting measurements...");
    Serial.println("----------------------------------------");
}

void loop() {
    if (sensorManager.update()) {
        const SensorData& data = sensorManager.getData();
        
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
    
    delay(1000); // Read every second
} 
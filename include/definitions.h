#pragma once

#include <cstdint>

/**
 * @struct SensorData
 * @brief Structure containing all sensor measurements
 */
struct SensorData {
    // Particle mass concentrations
    float pm1p0;         ///< PM1.0 concentration in µg/m³
    float pm2p5;         ///< PM2.5 concentration in µg/m³
    float pm4p0;         ///< PM4.0 concentration in µg/m³
    float pm10p0;        ///< PM10.0 concentration in µg/m³
    
    // Environmental parameters
    float humidity;      ///< Relative humidity in %
    float temperature;   ///< Temperature in Celsius
    
    // Air quality indices
    float vocIndex;      ///< VOC index
    float noxIndex;      ///< NOx index
    
    // Gas concentration
    float co2;           ///< CO2 concentration in ppm
    
    // Raw values
    int16_t rawHumidity;     ///< Scaled with factor 100: RH [%] = value / 100
    int16_t rawTemperature;  ///< Scaled with factor 200: T [°C] = value / 200
    uint16_t rawVOC;         ///< Raw VOC ticks without scale factor
    uint16_t rawNOx;         ///< Raw NOx ticks without scale factor
    uint16_t rawCO2;         ///< Not interpolated CO₂ concentration [ppm]
};

// Sensor threshold definitions
struct SensorThresholds {
    struct Temperature {
        static constexpr float BLUE_MAX = 10.0f;    // < 10°C
        static constexpr float GREEN_MAX = 30.0f;   // 10-30°C
        // > 30°C is red
    };

    struct Humidity {
        static constexpr float GREEN_MIN = 40.0f;   // 40-60%
        static constexpr float GREEN_MAX = 60.0f;
        static constexpr float ORANGE_MIN = 30.0f;  // 30-40% or 60-70%
        static constexpr float ORANGE_MAX = 70.0f;
        // < 30% or > 70% is red
    };

    struct CO2 {
        static constexpr float BLUE_MAX = 550.0f;   // < 550 ppm
        static constexpr float GREEN_MAX = 1000.0f;  // 550-1000 ppm
        static constexpr float ORANGE_MAX = 1600.0f; // 1000-1600 ppm
        // > 1600 ppm is red
    };

    struct PM25 {
        static constexpr float GREEN_MAX = 5.0f;    // < 5 µg/m³
        static constexpr float ORANGE_MAX = 35.0f;  // 5-35 µg/m³
        // > 35 µg/m³ is red
    };

    struct VOC {
        static constexpr float BLUE_MAX = 85.0f;    // < 85
        static constexpr float GREEN_MAX = 150.0f;  // 85-150
        static constexpr float ORANGE_MAX = 350.0f; // 150-350
        // > 350 is red
    };

    struct NOx {
        static constexpr float GREEN_MAX = 20.0f;    // ≤ 20
        static constexpr float ORANGE_MAX = 250.0f;   // 20-50
        // > 50 is red
    };
};

// SEN66
#define OVERTEMPERATURE              6

/*ESP32S3*/
#define PIN_LCD_BL                   38

#define PIN_LCD_D0                   39
#define PIN_LCD_D1                   40
#define PIN_LCD_D2                   41
#define PIN_LCD_D3                   42
#define PIN_LCD_D4                   45
#define PIN_LCD_D5                   46
#define PIN_LCD_D6                   47
#define PIN_LCD_D7                   48

#define PIN_POWER_ON                 15

#define PIN_LCD_RES                  5
#define PIN_LCD_CS                   6
#define PIN_LCD_DC                   7
#define PIN_LCD_WR                   8
#define PIN_LCD_RD                   9

#define PIN_BUTTON_LEFT              0
#define PIN_BUTTON_RIGHT             14
#define PIN_BAT_VOLT                 4

#define PIN_IIC_SCL                  17
#define PIN_IIC_SDA                  18

#define PIN_TOUCH_INT                16
#define PIN_TOUCH_RES                21

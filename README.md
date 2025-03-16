# IAQ Monitor

Indoor Air Quality Monitor using Lilygo T-Display S3 and Sensirion SEN66 sensor.

## Hardware Requirements

- Lilygo T-Display S3 (ESP32-S3)
- Sensirion SEN66 Environmental Sensor
- Battery (optional)

## Pin Connections

### Display (Pre-configured on T-Display S3)
- MOSI: GPIO35
- SCLK: GPIO36
- CS: GPIO34
- DC: GPIO37
- RST: GPIO38
- BL: GPIO33

### Sensor (I2C)
- SDA: GPIO18
- SCL: GPIO19

### Power Management
- Battery ADC: GPIO4
- Wake Button: GPIO0

## Features

- Temperature and Humidity monitoring
- VOC (Volatile Organic Compounds) detection
- NOx (Nitrogen Oxides) measurement
- Power-efficient operation with deep sleep support
- Modern UI with LVGL
- Battery monitoring

## Development Environment

This project uses PlatformIO for development. Two environments are provided:
- `development`: Debug output enabled, suitable for development
- `production`: Optimized for deployment with minimal debug output

### Building and Flashing

```bash
# Build for development
pio run -e development

# Build for production
pio run -e production

# Upload to device (development)
pio run -e development -t upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
├── src/                    # Source files
│   ├── main.cpp           # Main application entry
│   ├── iaq_monitor.cpp    # Main application logic
│   ├── sensor_manager.cpp # Sensor interface
│   ├── display_manager.cpp# Display and UI management
│   └── power_manager.cpp  # Power management
├── include/               # Header files
│   ├── iaq_monitor.h
│   ├── sensor_manager.h
│   ├── display_manager.h
│   └── power_manager.h
├── lib/                   # External libraries
├── platformio.ini         # PlatformIO configuration
└── README.md             # Project documentation
```

## Dependencies

- Arduino Core for ESP32
- LVGL (8.3.9)
- TFT_eSPI (2.5.31)
- Sensirion I2C SEN66 (1.2.0)

## Power Management

The device includes power-saving features:
- Deep sleep mode when battery is low
- Configurable wake-up sources (button/timer)
- Battery voltage monitoring
- Display brightness control

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request 
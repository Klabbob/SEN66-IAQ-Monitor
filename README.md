# IAQ Monitor

An Indoor Air Quality Monitor using the Sensirion SEN66 sensor with ESP32-S3.

## Features

- Real-time monitoring of:
  - Particle mass concentrations (PM1.0, PM2.5, PM4.0, PM10.0)
  - Environmental parameters (Temperature, Humidity)
  - Air quality indices (VOC Index, NOx Index)
  - Gas concentrations (CO2)
- Raw sensor data access for debugging and calibration
- Serial output with both processed and raw values

## Hardware Requirements

- LilyGo T-Display S3 board
- Sensirion SEN66 sensor
- I2C connections:
  - SDA: GPIO 18
  - SCL: GPIO 17
  - SEN66 I2C Address: 0x6B

## Software Requirements

- PlatformIO
- Arduino Core for ESP32
- Sensirion I2C SEN66 library (v1.2.0)
- Sensirion Core library (v0.3.0)

## Hardware Assembly

- Remove the blue and yellow cable from the connector
- cut the remaining 4 cables to a length of 110mm
- from the leftovers cut 75mm of the black and red wire

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/Klabbob/IAQ-Monitor.git
   ```

2. Open the project in PlatformIO

3. Install dependencies:
   ```bash
   pio lib install
   ```

4. Build and upload to your device

## Sensor Data

### Processed Values

- Particle Mass Concentrations (μg/m³):
  - PM1.0
  - PM2.5
  - PM4.0
  - PM10.0

- Environmental Parameters:
  - Temperature (°C)
  - Humidity (%RH)

- Air Quality Indices (0-500):
  - VOC Index
  - NOx Index

- Gas Concentration:
  - CO2 (ppm)

### Raw Values

- Environmental Parameters:
  - Raw Humidity: Scaled with factor 100 (RH [%] = value / 100)
  - Raw Temperature: Scaled with factor 200 (T [°C] = value / 200)

- Gas Sensors:
  - Raw VOC: Raw ticks without scale factor
  - Raw NOx: Raw ticks without scale factor
  - Raw CO2: Not interpolated CO₂ concentration [ppm]

### Raw Value Notes

- If humidity or temperature values are unknown, 0x7FFF is returned
- If VOC, NOx, or CO2 values are unknown, 0xFFFF is returned
- During the first 10-11 seconds after power-on or device reset, NOx will be 0xFFFF
- During the first 5-6 seconds after power-on or device reset, CO2 will be 0xFFFF

## Serial Output

The device outputs data in the following format:

```
Processed Sensor Readings:
----------------------------------------
Particle Mass Concentrations:
  PM1.0: X.XX μg/m³
  PM2.5: X.XX μg/m³
  PM4.0: X.XX μg/m³
  PM10.0: X.XX μg/m³

Environmental Parameters:
  Temperature: XX.XX °C
  Humidity: XX.XX %RH

Air Quality Indices:
  VOC Index: XXX (0-500)
  NOx Index: XXX (0-500)

Gas Concentration:
  CO2: XXX ppm

Raw Sensor Values:
----------------------------------------
Environmental Parameters (Raw):
  Humidity: XXXX (raw) = XX.XX %RH
  Temperature: XXXX (raw) = XX.XX °C

Gas Sensors (Raw):
  VOC: XXXX (raw ticks)
  NOx: XXXX (raw ticks)
  CO2: XXXX (not interpolated [ppm])
----------------------------------------
```

## Development

### UI
The UI is built with SquareLine Studio. Due to the limitiations of the free license tier, the setting screens are designed in a separate project. The projects can be found in the SquareLine Studio UI Source folder. 

It is necessary to manually assembly the two ui exports. The main screens will be directly exported to the ui folder, the setting screens to the ui_Settings folder. 

1. Copy over all screen files (ui_XYZScreen.c) to ui from ui_Settings
2. Copy over all image files (ui_img_XYZ.c) that are not already in ui
3. Copy all unique parts of ui.c and ui.h in ui_Settings into the respective files in ui
4. Repeat the same for the filelist.txt and CMakeLists.text files.

Contgratulations, you merged both projects. If you add new screens, make sure to update display_task.h and display_task.cpp. If you add a lot of new stuff, you might need to increase LV_MEM_SIZE in lv_conf.h

### Building

```bash
pio run
```

### Uploading

```bash
pio run -t upload
```

### Monitoring

```bash
pio device monitor
```

## License

This project is licensed under the MIT License - see the LICENSE file for details. 
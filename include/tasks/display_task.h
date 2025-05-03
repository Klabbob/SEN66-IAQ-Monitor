#pragma once

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui/ui.h>
#include <FreeRTOS.h>
#include <task.h>
#include <Preferences.h>
#include "definitions.h"

/**
 * @class DisplayTask
 * @brief Manages the display hardware and LVGL interface for the IAQ Monitor
 * 
 * This class implements a singleton pattern to ensure only one display instance exists.
 * It handles all display-related operations including initialization, screen updates,
 * and tile value updates.
 */
class DisplayTask {
public:
    /**
     * @brief Get the singleton instance of DisplayTask
     * @return Reference to the DisplayTask instance
     */
    static DisplayTask& getInstance();

    /**
     * @brief Static task function to be run in FreeRTOS task
     * @param parameter Task parameters (unused)
     */
    static void displayTask(void* parameter);

    // Task handle
    static TaskHandle_t xDisplayTaskHandle;

    // Button handlers
    void handleLeftButtonPress();
    void handleRightButtonPress();
    void handleLeftButtonLongPress();
    void handleRightButtonLongPress();

    /**
     * @brief Set the display brightness using PWM
     * @param brightness Value from 0 to 255
     */
    void setDisplayBrightness(uint8_t brightness);

private:
    // Private constructor for singleton
    DisplayTask();
    ~DisplayTask() = default;
    
    // Delete copy constructor and assignment operator
    DisplayTask(const DisplayTask&) = delete;
    DisplayTask& operator=(const DisplayTask&) = delete;

    // Settings mode management
    void enterSettingsMode();
    void exitSettingsMode();

    // Display dimensions
    static constexpr uint16_t kScreenWidth = 170;
    static constexpr uint16_t kScreenHeight = 320;
    static constexpr uint16_t kBufferSize = kScreenWidth * kScreenHeight / 10;  // Full screen buffer
    
    // LEDC PWM configuration
    static constexpr uint8_t kLEDCChannel = 0;        // LEDC channel 0
    static constexpr uint8_t kLEDCResolution = 8;     // 8-bit resolution (0-255)
    static constexpr uint32_t kLEDCFrequency = 5000;  // 5kHz PWM frequency
    
    // Ring buffer configuration
    static constexpr size_t kRingBufferSize  = 150;         // Ring buffer Size
    static constexpr size_t kMidTermBufferSize = 24;        // 24 seconds of data
    static constexpr size_t kLongTermBufferSize = 24;      // 576 seconds of data by using kMidTermBufferSize combined with kLongTermBufferSize
    
    // Chart display modes
    enum class ChartDisplayMode {
        ShortTerm,    // 2.5 minutes of data
        MidTerm,      // 60 minutes of data
        LongTerm      // 24 hours of data
    };
    
    ChartDisplayMode currentDisplayMode = ChartDisplayMode::ShortTerm;

    // Settings state storage
    int savedFRCTargetValue = 0;
    int savedAltitudeValue = 0;
    ChartDisplayMode savedChartDisplayMode = ChartDisplayMode::ShortTerm;
    bool savedBrightness100 = false;
    bool savedBrightness75 = false;
    bool savedBrightness50 = false;
    bool savedBrightness25 = false;
    int32_t savedFRCSetValue = 0;
    String savedFRCTitle = "";
    String savedFRCUnit = "";

    // Ring buffers for IAQ parameters
    struct ParameterBuffers {
        // Short-term 
        lv_coord_t short_term_ring_buffer[kRingBufferSize];
        
        // Mid-term 
        lv_coord_t mid_term_ring_buffer[kRingBufferSize];
        float mid_term_sum = 0.0f;
        uint8_t mid_term_count = 0;
        
        // Long-term
        lv_coord_t long_term_ring_buffer[kRingBufferSize];
        float long_term_sum = 0.0f;
        uint8_t long_term_count = 0;
    };
    
    ParameterBuffers pm1_buffers;
    ParameterBuffers pm2p5_buffers;
    ParameterBuffers pm4_buffers;
    ParameterBuffers pm10_buffers;
    ParameterBuffers co2_buffers;
    ParameterBuffers voc_buffers;
    ParameterBuffers nox_buffers;
    ParameterBuffers temp_buffers;
    ParameterBuffers rh_buffers;
    
    // Helper functions for buffer management
    void updateParameterBuffer(ParameterBuffers* buffers, float value);
    
    // Chart series pointers
    lv_chart_series_t* pm1_series;
    lv_chart_series_t* pm2p5_series;
    lv_chart_series_t* pm4_series;
    lv_chart_series_t* pm10_series;
    lv_chart_series_t* co2_series;
    lv_chart_series_t* voc_series;
    lv_chart_series_t* nox_series;
    lv_chart_series_t* temp_series;
    lv_chart_series_t* rh_series;

    // Display hardware
    TFT_eSPI tft;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t display_buffer[kBufferSize];

    // State machine states
    enum class ScreenState {
        MainScreen,
        PMScreen,
        CO2Screen,
        VOCScreen,
        NOxScreen,
        TempScreen,
        RHScreen,
        SettingsScreen,
        FRCScreen,
        AltitudeScreen,
        ChartTimeScreen,
        BrightnessScreen
    };

    // Current state
    ScreenState currentState = ScreenState::MainScreen;
    bool inSettingsMode = false;
    bool processing = false;
    bool frcconfirmed = false;

    // Screen management
    uint8_t currentScreenIndex = 0;
    #define NUM_SCREENS 12  // Main, PM, CO2, VOC, NOx, Temp, RH, FRC, Settings, Brightness, ChartTime, Altitude

    // Chart range labels
    lv_obj_t* pm_min_label;
    lv_obj_t* pm_max_label;
    lv_obj_t* co2_min_label;
    lv_obj_t* co2_max_label;
    lv_obj_t* voc_min_label;
    lv_obj_t* voc_max_label;
    lv_obj_t* nox_min_label;
    lv_obj_t* nox_max_label;
    lv_obj_t* temp_min_label;
    lv_obj_t* temp_max_label;
    lv_obj_t* rh_min_label;
    lv_obj_t* rh_max_label;

    // Screen switching helper
    void switchScreen(uint8_t screenIndex);

    // Indicator states
    enum class IndicatorState {
        Green,
        Orange,
        Red,
        Blue
    };

    /**
     * @brief Initialize the display hardware and LVGL
     */
    void init_display();

    /**
     * @brief Configure chart with antialiasing
     * @param chart The chart object to configure
     */
    void configure_chart_antialiasing(lv_obj_t* chart);

    /**
     * @brief Flush the display buffer to the screen
     * @param disp LVGL display driver
     * @param area Area to update
     * @param color_p Color data to display
     */
    static void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);

    /**
     * @brief Update a tile's value with a float
     * @param tile The tile object to update
     * @param value The new value to display
     */
    void update_tile_value(lv_obj_t* tile, float value);

    /**
     * @brief Update a tile's indicator state
     * @param tile The tile object to update
     * @param state The new indicator state
     */
    void update_indicator_state(lv_obj_t* tile, IndicatorState state);

    /**
     * @brief Update all tile indicators based on sensor data
     * @param data The sensor data to evaluate
     */
    void update_all_indicators(const SensorData& data);

    /**
     * @brief Update a label's text and color based on a value
     * @param label The label object to update
     * @param value The value to display
     * @param decimals The number of decimal places to display
     */
    void update_value_text(lv_obj_t* label, float value, uint8_t decimals);

    /**
     * @brief Initialize all buffers
     */
    void init_buffers();
    
    /**
     * @brief Update a ring buffer with a new value
     * @param buffer The ring buffer to update
     * @param value The new value to add
     */
    void update_ring_buffer(lv_coord_t* buffer, float value);
    
    /**
     * @brief Validate and scale a sensor value based on its type
     * @param buffer The ring buffer the value will be stored in
     * @param value The raw sensor value to validate and scale
     * @return The scaled value, or -1.0f if the value is invalid
     */
    float valid_value(lv_coord_t* buffer, float value);
    
    /**
     * @brief Update chart series with ring buffer values
     * @param chart The chart object
     * @param series The series to update
     * @param buffer The ring buffer containing the values
     * @param series2 Optional second series (for PM chart)
     * @param buffer2 Optional second buffer (for PM chart)
     * @param series3 Optional third series (for PM chart)
     * @param buffer3 Optional third buffer (for PM chart)
     * @param series4 Optional fourth series (for PM chart)
     * @param buffer4 Optional fourth buffer (for PM chart)
     */
    void update_chart_series(lv_obj_t* chart, lv_chart_series_t* series, ParameterBuffers* buffer,
                           lv_chart_series_t* series2 = nullptr, ParameterBuffers* buffer2 = nullptr,
                           lv_chart_series_t* series3 = nullptr, ParameterBuffers* buffer3 = nullptr,
                           lv_chart_series_t* series4 = nullptr, ParameterBuffers* buffer4 = nullptr);

    /**
     * @brief Calculate and set adaptive range for a chart based on its data
     * @param chart The chart object
     * @param buffer The data buffer
     * @param default_min Default minimum value
     * @param default_max Default maximum value
     * @param min_spread Minimum required spread between min and max values
     * @param buffer2 Optional second buffer (for PM chart)
     * @param buffer3 Optional third buffer (for PM chart)
     * @param buffer4 Optional fourth buffer (for PM chart)
     */
    void calculate_adaptive_range(lv_obj_t* chart, lv_coord_t* buffer, 
                                float default_min, float default_max, float min_spread,
                                lv_coord_t* buffer2 = nullptr, lv_coord_t* buffer3 = nullptr,
                                lv_coord_t* buffer4 = nullptr);

    /**
     * @brief Cycle through the chart display modes
     * @param up Whether to cycle up or down
     * @param reset Whether to reset the display mode to the saved value
     */
    void cycleChartDisplayMode(bool up = false, bool reset = false);
};
#pragma once

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui/ui.h>
#include <FreeRTOS.h>
#include <task.h>
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

    /**
     * @brief Switch to the next screen
     */
    static void nextScreen();

    /**
     * @brief Switch to the previous screen
     */
    static void previousScreen();

    // Task handle
    static TaskHandle_t xDisplayTaskHandle;

private:
    // Private constructor for singleton
    DisplayTask();
    ~DisplayTask() = default;
    
    // Delete copy constructor and assignment operator
    DisplayTask(const DisplayTask&) = delete;
    DisplayTask& operator=(const DisplayTask&) = delete;

    // Display dimensions
    static constexpr uint16_t kScreenWidth = 170;
    static constexpr uint16_t kScreenHeight = 320;
    static constexpr uint16_t kBufferSize = kScreenWidth * kScreenHeight;  // Full screen buffer
    static constexpr size_t kRingBufferSize = 150;

    // Display hardware
    TFT_eSPI tft;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t display_buffer[kBufferSize];

    // Screen management
    uint8_t currentScreenIndex = 0;
    #define NUM_SCREENS 8  // Main, PM, CO2, VOC, NOx, Temp, RH, FRC

    // Ring buffers for IAQ parameters
    lv_coord_t pm1_ring_buffer[kRingBufferSize];
    lv_coord_t pm2p5_ring_buffer[kRingBufferSize];
    lv_coord_t pm4_ring_buffer[kRingBufferSize];
    lv_coord_t pm10_ring_buffer[kRingBufferSize];
    lv_coord_t co2_ring_buffer[kRingBufferSize];
    lv_coord_t voc_ring_buffer[kRingBufferSize];
    lv_coord_t nox_ring_buffer[kRingBufferSize];
    lv_coord_t temp_ring_buffer[kRingBufferSize];
    lv_coord_t rh_ring_buffer[kRingBufferSize];
    
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
     * @param is_integer Whether to display the value as an integer
     */
    void update_value_text(lv_obj_t* label, float value, bool is_integer = false);

    /**
     * @brief Initialize all ring buffers with -1
     */
    void init_ring_buffers();
    
    /**
     * @brief Update a ring buffer with a new value
     * @param buffer The ring buffer to update
     * @param value The new value to add
     */
    void update_ring_buffer(lv_coord_t* buffer, float value);
    
    /**
     * @brief Update chart series with ring buffer values
     * @param chart The chart object
     * @param series The series to update
     * @param buffer The ring buffer containing the values
     */
    void update_chart_series(lv_obj_t* chart, lv_chart_series_t* series, lv_coord_t* buffer);

    /**
     * @brief Calculate and set adaptive range for a chart based on its data
     * @param chart The chart object
     * @param buffer The data buffer
     * @param default_min Default minimum value
     * @param default_max Default maximum value
     * @param min_spread Minimum required spread between min and max values
     */
    void calculate_adaptive_range(lv_obj_t* chart, lv_coord_t* buffer, 
                                float default_min, float default_max, float min_spread);
};
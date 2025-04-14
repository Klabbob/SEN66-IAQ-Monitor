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
    static constexpr uint16_t kBufferSize = kScreenWidth * kScreenHeight / 10;

    // Display hardware
    TFT_eSPI tft;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t display_buffer[kBufferSize];

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
};
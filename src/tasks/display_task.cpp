#include "tasks/display_task.h"
#include "tasks/live_data_manager.h"
#include "tasks/i2c_scan_task.h"
#include <cstdio>

// Initialize static member
TaskHandle_t DisplayTask::xDisplayTaskHandle = nullptr;

// Queue handle for receiving data
static QueueHandle_t xDataQueue = nullptr;

// Initialize TFT display in constructor
DisplayTask::DisplayTask() : tft(kScreenWidth, kScreenHeight) {}

DisplayTask& DisplayTask::getInstance() {
    static DisplayTask instance;
    return instance;
}

void DisplayTask::displayTask(void* parameter) {
    auto& instance = getInstance();
    instance.init_display();
    
    // Subscribe to LiveDataManager
    if (!LiveDataManager::getInstance().subscribe(xTaskGetCurrentTaskHandle(), &xDataQueue)) {
        Serial.println("DisplayTask: Failed to subscribe to LiveDataManager!");
        vTaskDelete(NULL);
        return;
    }
    
    QueueMessage message;
    while (true) {        
        // Check for new sensor data
        if (xQueueReceive(xDataQueue, &message, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) == pdTRUE) {
            const SensorData& data = message.data;

            if (data.runtime_ticks == 1) {
                // Initialize buffers to reset all values as the sensor just started
                instance.init_buffers();
            }
            
            // Update ring buffers with new data
            instance.updateParameterBuffer(&instance.pm1_buffers, data.pm1p0);
            instance.updateParameterBuffer(&instance.pm2p5_buffers, data.pm2p5);
            instance.updateParameterBuffer(&instance.pm4_buffers, data.pm4p0);
            instance.updateParameterBuffer(&instance.pm10_buffers, data.pm10p0);
            instance.updateParameterBuffer(&instance.co2_buffers, data.co2);
            instance.updateParameterBuffer(&instance.voc_buffers, data.vocIndex);
            instance.updateParameterBuffer(&instance.nox_buffers, data.noxIndex);
            instance.updateParameterBuffer(&instance.temp_buffers, data.temperature);
            instance.updateParameterBuffer(&instance.rh_buffers, data.humidity);
            
            // Update chart series with new ring buffer values
            instance.update_chart_series(ui_PMScreen_PMChart, instance.pm1_series, &instance.pm1_buffers,
                                        instance.pm2p5_series, &instance.pm2p5_buffers,
                                        instance.pm4_series, &instance.pm4_buffers,
                                        instance.pm10_series, &instance.pm10_buffers);
            instance.update_chart_series(ui_CO2Screen_CO2Chart, instance.co2_series, &instance.co2_buffers);
            instance.update_chart_series(ui_VOCScreen_VOCChart, instance.voc_series, &instance.voc_buffers);
            instance.update_chart_series(ui_NOxScreen_NOxChart, instance.nox_series, &instance.nox_buffers);
            instance.update_chart_series(ui_TempScreen_TempChart, instance.temp_series, &instance.temp_buffers);
            instance.update_chart_series(ui_RHScreen_RHChart, instance.rh_series, &instance.rh_buffers);
            
            // Update Main Screen tiles
            instance.update_tile_value(ui_MainScreen_TilePM, data.pm2p5);    // PM2.5
            instance.update_tile_value(ui_MainScreen_TileRH, data.humidity); // Relative Humidity
            instance.update_tile_value(ui_MainScreen_TileT, data.temperature); // Temperature
            instance.update_tile_value(ui_MainScreen_TileNOx, data.noxIndex); // NOx Index
            instance.update_tile_value(ui_MainScreen_TileVOC, data.vocIndex); // VOC Index
            instance.update_tile_value(ui_MainScreen_TileCO2, data.co2);     // CO2
            
            // Update PM Screen values
            instance.update_value_text(ui_PMScreen_Value, data.pm1p0, PM_DECIMALS);   // PM1.0
            instance.update_value_text(ui_PMScreen_Value1, data.pm2p5, PM_DECIMALS);  // PM2.5
            instance.update_value_text(ui_PMScreen_Value2, data.pm4p0, PM_DECIMALS);  // PM4.0
            instance.update_value_text(ui_PMScreen_Value3, data.pm10p0, PM_DECIMALS); // PM10.0
            
            // Update CO2 Screen value
            instance.update_value_text(ui_CO2Screen_Value, data.co2, CO2_DECIMALS);
            
            // Update VOC Screen value
            instance.update_value_text(ui_VOCScreen_Value, data.vocIndex, VOC_DECIMALS);
            
            // Update NOx Screen value
            instance.update_value_text(ui_NOxScreen_Value, data.noxIndex, NOX_DECIMALS);
            
            // Update Temperature Screen value
            instance.update_value_text(ui_TempScreen_Value, data.temperature, TEMP_DECIMALS);
            
            // Update RH Screen value
            instance.update_value_text(ui_RHScreen_Value, data.humidity, RH_DECIMALS);
            
            // Update indicator states
            instance.update_all_indicators(data);

            //Update Runtime on FRC Screen
            if (data.runtime_ticks >= 120) {
                lv_obj_clear_state(ui_FRCScreen_LabelRuntime, LV_STATE_USER_1);
            } else {
                lv_obj_add_state(ui_FRCScreen_LabelRuntime, LV_STATE_USER_1);
            }
            lv_label_set_text_fmt(ui_FRCScreen_LabelRuntime, "%lu sec", data.runtime_ticks);
        }

        // Handle LVGL tasks
        lv_task_handler();
        
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void DisplayTask::my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    auto& instance = getInstance();
    instance.tft.startWrite();
    instance.tft.setAddrWindow(area->x1, area->y1, w, h);
    instance.tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    instance.tft.endWrite();

    lv_disp_flush_ready(disp);
}

void DisplayTask::update_tile_value(lv_obj_t* tile, float value) {
    char buffer[16];
    
    // Check for unknown values based on parameter type
    bool is_unknown = false;
    
    if (tile == ui_MainScreen_TilePM) {
        // PM2.5: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::PM25::MIN || value > SensorThresholds::PM25::MAX);
    } else if (tile == ui_MainScreen_TileCO2) {
        // CO2: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::CO2::MIN || value > SensorThresholds::CO2::MAX);
    } else if (tile == ui_MainScreen_TileVOC) {
        // VOC Index: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::VOC::MIN || value > SensorThresholds::VOC::MAX);
    } else if (tile == ui_MainScreen_TileNOx) {
        // NOx Index: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::NOx::MIN || value > SensorThresholds::NOx::MAX);
    } else if (tile == ui_MainScreen_TileT) {
        // Temperature: Unknown if value is outside MIN to MAX
        is_unknown = (value < SensorThresholds::Temperature::MIN || value > SensorThresholds::Temperature::MAX);
    } else if (tile == ui_MainScreen_TileRH) {
        // Humidity: Unknown if value is outside MIN to MAX
        is_unknown = (value < SensorThresholds::Humidity::MIN || value > SensorThresholds::Humidity::MAX);
    }
    
    if (is_unknown) {
        lv_label_set_text(ui_comp_get_child(tile, UI_COMP_TILE_VALUE), "-");
        return;
    }
    
    // Special formatting for different measurements
    if (tile == ui_MainScreen_TilePM && value >= 100.0f) {
        // PM2.5 >= 100: show as integer
        snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
    } else if (tile == ui_MainScreen_TileNOx || 
               tile == ui_MainScreen_TileVOC || 
               tile == ui_MainScreen_TileCO2) {
        // NOx, VOC, and CO2: always show as integer
        snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
    } else {
        // Default: show with one decimal place
        snprintf(buffer, sizeof(buffer), "%.1f", value);
    }
    
    lv_label_set_text(ui_comp_get_child(tile, UI_COMP_TILE_VALUE), buffer);
}

void DisplayTask::configure_chart_antialiasing(lv_obj_t* chart) {
    // Enable enhanced antialiasing for the chart lines
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);  // Thin lines
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_line_opa(chart, LV_OPA_COVER, LV_PART_ITEMS);
    
    // Configure the chart for smoother rendering
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(chart, 150);  // Original point count
    
    // Add minimal padding to prevent lines from touching the edges
    lv_obj_set_style_pad_all(chart, 1, LV_PART_MAIN);
}

void DisplayTask::init_buffers() {
    // Initialize all buffers
    for (size_t i = 0; i < kRingBufferSize; i++) {
        pm1_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm1_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm1_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm2p5_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm2p5_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm2p5_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm4_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm4_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm4_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm10_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm10_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        pm10_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        co2_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;    
        co2_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        co2_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        voc_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        voc_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        voc_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        nox_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        nox_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        nox_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        temp_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        temp_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        temp_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        rh_buffers.short_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        rh_buffers.mid_term_ring_buffer[i] = LV_CHART_POINT_NONE;
        rh_buffers.long_term_ring_buffer[i] = LV_CHART_POINT_NONE;
    }
    pm1_buffers.mid_term_sum = 0.0f;
    pm1_buffers.mid_term_count = 0;
    pm1_buffers.long_term_sum = 0.0f;
    pm1_buffers.long_term_count = 0;
    pm2p5_buffers.mid_term_sum = 0.0f;
    pm2p5_buffers.mid_term_count = 0;
    pm2p5_buffers.long_term_sum = 0.0f;
    pm2p5_buffers.long_term_count = 0;
    pm4_buffers.mid_term_sum = 0.0f;
    pm4_buffers.mid_term_count = 0;
    pm4_buffers.long_term_sum = 0.0f;
    pm4_buffers.long_term_count = 0;
    pm10_buffers.mid_term_sum = 0.0f;
    pm10_buffers.mid_term_count = 0;
    pm10_buffers.long_term_sum = 0.0f;
    pm10_buffers.long_term_count = 0;
    co2_buffers.mid_term_sum = 0.0f;
    co2_buffers.mid_term_count = 0;
    co2_buffers.long_term_sum = 0.0f;
    co2_buffers.long_term_count = 0;
    voc_buffers.mid_term_sum = 0.0f;
    voc_buffers.mid_term_count = 0;
    voc_buffers.long_term_sum = 0.0f;
    voc_buffers.long_term_count = 0;
    nox_buffers.mid_term_sum = 0.0f;
    nox_buffers.mid_term_count = 0;
    nox_buffers.long_term_sum = 0.0f;
    nox_buffers.long_term_count = 0;
    temp_buffers.mid_term_sum = 0.0f;
    temp_buffers.mid_term_count = 0;
    temp_buffers.long_term_sum = 0.0f;
    temp_buffers.long_term_count = 0;
    rh_buffers.mid_term_sum = 0.0f;
    rh_buffers.mid_term_count = 0;
    rh_buffers.long_term_sum = 0.0f;
    rh_buffers.long_term_count = 0;
}

void DisplayTask::update_ring_buffer(lv_coord_t* buffer, float value) {
    // Shift all values one position to the left
    for (size_t i = 0; i < kRingBufferSize - 1; i++) {
        buffer[i] = buffer[i + 1];
    }
    
    float scaled_value = valid_value(buffer, value);
    
    // Add new value at the end, or LV_CHART_POINT_NONE if invalid
    if (scaled_value != -1.0f){ // Valid value
        buffer[kRingBufferSize - 1] = static_cast<lv_coord_t>(scaled_value);
    }
    else{ // Invalid value
        buffer[kRingBufferSize - 1] = LV_CHART_POINT_NONE;
    }
}

float DisplayTask::valid_value(lv_coord_t* buffer, float value){
    // Check for unknown values based on parameter type
    float scaled_value = value;
    
    if (buffer == pm1_buffers.short_term_ring_buffer || buffer == pm2p5_buffers.short_term_ring_buffer || 
        buffer == pm4_buffers.short_term_ring_buffer || buffer == pm10_buffers.short_term_ring_buffer) {
        // PM values: Unknown if value is negative or exceeds MAX
        if (value < SensorThresholds::PM25::MIN || value > SensorThresholds::PM25::MAX){
            scaled_value = -1.0f;
        }
        else{
            // Scale to maintain PM_DECIMALS decimal places
            scaled_value = value * powf(10, PM_DECIMALS);
        }
    } else if (buffer == co2_buffers.short_term_ring_buffer) {
        // CO2: Unknown if value is negative or exceeds MAX
        if (value < SensorThresholds::CO2::MIN || value > SensorThresholds::CO2::MAX){
            scaled_value = -1.0f;
        }
        else{
            // Scale to maintain CO2_DECIMALS decimal places
            scaled_value = value * powf(10, CO2_DECIMALS);
        }
    } else if (buffer == voc_buffers.short_term_ring_buffer) {
        // VOC Index: Unknown if value is negative or exceeds MAX
        if (value < SensorThresholds::VOC::MIN || value > SensorThresholds::VOC::MAX){
            scaled_value = -1.0f;
        }
        else{
            // Scale to maintain VOC_DECIMALS decimal places
            scaled_value = value * powf(10, VOC_DECIMALS);
        }
    } else if (buffer == nox_buffers.short_term_ring_buffer) {
        // NOx Index: Unknown if value is negative or exceeds MAX
        if (value < SensorThresholds::NOx::MIN || value > SensorThresholds::NOx::MAX){
            scaled_value = -1.0f;
        }
        else{
            // Scale to maintain NOX_DECIMALS decimal places
            scaled_value = value * powf(10, NOX_DECIMALS);
        }
    } else if (buffer == temp_buffers.short_term_ring_buffer) {
        // Temperature: Unknown if value is outside MIN to MAX
        if (value < SensorThresholds::Temperature::MIN || value > SensorThresholds::Temperature::MAX){
            scaled_value = -1.0f;
        }
        else{
            // Scale to maintain TEMP_DECIMALS decimal places
            scaled_value = value * powf(10, TEMP_DECIMALS);
        }
    } else if (buffer == rh_buffers.short_term_ring_buffer) {
        // Humidity: Unknown if value is outside MIN to MAX
        if (value < SensorThresholds::Humidity::MIN || value > SensorThresholds::Humidity::MAX){
            scaled_value = -1.0f;
        }
        else{
            // Scale to maintain RH_DECIMALS decimal places
            scaled_value = value * powf(10, RH_DECIMALS);
        }
    }
    return scaled_value;
}

// Helper function to calculate adaptive range for a chart
void DisplayTask::calculate_adaptive_range(lv_obj_t* chart, lv_coord_t* buffer, 
                                         float default_min, float default_max, float min_spread,
                                         lv_coord_t* buffer2, lv_coord_t* buffer3, lv_coord_t* buffer4) {
    float min_val = 20000.0f;
    float max_val = -200.0f;
    bool has_valid_data = false;

    // Calculate scaled minimum spread and decimals based on chart type
    float scaled_min_spread = min_spread;
    uint8_t decimals = 0;
    if (chart == ui_PMScreen_PMChart) {
        scaled_min_spread = min_spread * powf(10, PM_DECIMALS);
        decimals = PM_DECIMALS;
    } else if (chart == ui_CO2Screen_CO2Chart) {
        scaled_min_spread = min_spread * powf(10, CO2_DECIMALS);
        decimals = CO2_DECIMALS;
    } else if (chart == ui_VOCScreen_VOCChart) {
        scaled_min_spread = min_spread * powf(10, VOC_DECIMALS);
        decimals = VOC_DECIMALS;
    } else if (chart == ui_NOxScreen_NOxChart) {
        scaled_min_spread = min_spread * powf(10, NOX_DECIMALS);
        decimals = NOX_DECIMALS;
    } else if (chart == ui_TempScreen_TempChart) {
        scaled_min_spread = min_spread * powf(10, TEMP_DECIMALS);
        decimals = TEMP_DECIMALS;
    } else if (chart == ui_RHScreen_RHChart) {
        scaled_min_spread = min_spread * powf(10, RH_DECIMALS);
        decimals = RH_DECIMALS;
    }

    // Find min and max values in the buffer(s)
    if (chart == ui_PMScreen_PMChart && buffer2 && buffer3 && buffer4) {
        // For PM chart, check all four buffers
        for (size_t i = 0; i < kRingBufferSize; i++) {
            if (buffer[i] != LV_CHART_POINT_NONE) {
                float val = static_cast<float>(buffer[i]);
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
                has_valid_data = true;
            }
            if (buffer4[i] != LV_CHART_POINT_NONE) {
                float val = static_cast<float>(buffer4[i]);
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
                has_valid_data = true;
            }
        }
    } else {
        // For other charts, use the provided buffer
        for (size_t i = 0; i < kRingBufferSize; i++) {
            if (buffer[i] != LV_CHART_POINT_NONE) {
                float val = static_cast<float>(buffer[i]);
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
                has_valid_data = true;
            }
        }
    }

    // If no valid data, use default range (already scaled)
    if (!has_valid_data) {
        min_val = default_min * powf(10, decimals);
        max_val = default_max * powf(10, decimals);
    }

    // Ensure minimum spread while respecting the minimum value constraint
    if (max_val - min_val < scaled_min_spread) {
        float center = (min_val + max_val) / 2.0f;
        min_val = center - scaled_min_spread / 2.0f;
        max_val = center + scaled_min_spread / 2.0f;
        if (min_val < 0 and chart != ui_TempScreen_TempChart) {
            min_val = 0;
            max_val = scaled_min_spread;
        }
    }

    // Set the chart range
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 
                      static_cast<lv_coord_t>(min_val), 
                      static_cast<lv_coord_t>(max_val));

    // Update the range labels with proper decimal scaling
    char min_buffer[16];
    char max_buffer[16];
    float scale_factor = 1.0f / powf(10, decimals);
    
    snprintf(min_buffer, sizeof(min_buffer), "%.*f", decimals, min_val * scale_factor);
    snprintf(max_buffer, sizeof(max_buffer), "%.*f", decimals, max_val * scale_factor);

    // Update the appropriate labels based on chart type
    if (chart == ui_PMScreen_PMChart) {
        lv_label_set_text(pm_min_label, min_buffer);
        lv_label_set_text(pm_max_label, max_buffer);
    } else if (chart == ui_CO2Screen_CO2Chart) {
        lv_label_set_text(co2_min_label, min_buffer);
        lv_label_set_text(co2_max_label, max_buffer);
    } else if (chart == ui_VOCScreen_VOCChart) {
        lv_label_set_text(voc_min_label, min_buffer);
        lv_label_set_text(voc_max_label, max_buffer);
    } else if (chart == ui_NOxScreen_NOxChart) {
        lv_label_set_text(nox_min_label, min_buffer);
        lv_label_set_text(nox_max_label, max_buffer);
    } else if (chart == ui_TempScreen_TempChart) {
        lv_label_set_text(temp_min_label, min_buffer);
        lv_label_set_text(temp_max_label, max_buffer);
    } else if (chart == ui_RHScreen_RHChart) {
        lv_label_set_text(rh_min_label, min_buffer);
        lv_label_set_text(rh_max_label, max_buffer);
    }
}

void DisplayTask::update_chart_series(lv_obj_t* chart, lv_chart_series_t* series, ParameterBuffers* buffer,
                                    lv_chart_series_t* series2, ParameterBuffers* buffer2,
                                    lv_chart_series_t* series3, ParameterBuffers* buffer3,
                                    lv_chart_series_t* series4, ParameterBuffers* buffer4) {
    // Select the appropriate buffer based on current display mode
    lv_coord_t* active_buffer;
    lv_coord_t* active_buffer2 = nullptr;
    lv_coord_t* active_buffer3 = nullptr;
    lv_coord_t* active_buffer4 = nullptr;
    
    switch (currentDisplayMode) {
        case ChartDisplayMode::ShortTerm:
            active_buffer = buffer->short_term_ring_buffer;
            if (chart == ui_PMScreen_PMChart && series2 && buffer2 && series3 && buffer3 && series4 && buffer4) {
                active_buffer2 = buffer2->short_term_ring_buffer;
                active_buffer3 = buffer3->short_term_ring_buffer;
                active_buffer4 = buffer4->short_term_ring_buffer;
            }
            break;
        case ChartDisplayMode::MidTerm:
            active_buffer = buffer->mid_term_ring_buffer;
            if (chart == ui_PMScreen_PMChart && series2 && buffer2 && series3 && buffer3 && series4 && buffer4) {
                active_buffer2 = buffer2->mid_term_ring_buffer;
                active_buffer3 = buffer3->mid_term_ring_buffer;
                active_buffer4 = buffer4->mid_term_ring_buffer;
            }
            break;
        case ChartDisplayMode::LongTerm:
            active_buffer = buffer->long_term_ring_buffer;
            if (chart == ui_PMScreen_PMChart && series2 && buffer2 && series3 && buffer3 && series4 && buffer4) {
                active_buffer2 = buffer2->long_term_ring_buffer;
                active_buffer3 = buffer3->long_term_ring_buffer;
                active_buffer4 = buffer4->long_term_ring_buffer;
            }
            break;
    }

    // Update the chart range based on the buffer data
    if (chart == ui_PMScreen_PMChart) {
        calculate_adaptive_range(chart, active_buffer, 
                               ChartRanges::PM::DEFAULT_MIN, 
                               ChartRanges::PM::DEFAULT_MAX, 
                               ChartRanges::PM::MIN_SPREAD,
                               active_buffer2, active_buffer3, active_buffer4);
    } else if (chart == ui_CO2Screen_CO2Chart) {
        calculate_adaptive_range(chart, active_buffer, 
                               ChartRanges::CO2::DEFAULT_MIN, 
                               ChartRanges::CO2::DEFAULT_MAX, 
                               ChartRanges::CO2::MIN_SPREAD);
    } else if (chart == ui_VOCScreen_VOCChart) {
        calculate_adaptive_range(chart, active_buffer, 
                               ChartRanges::VOC::DEFAULT_MIN, 
                               ChartRanges::VOC::DEFAULT_MAX, 
                               ChartRanges::VOC::MIN_SPREAD);
    } else if (chart == ui_NOxScreen_NOxChart) {
        calculate_adaptive_range(chart, active_buffer, 
                               ChartRanges::NOx::DEFAULT_MIN, 
                               ChartRanges::NOx::DEFAULT_MAX, 
                               ChartRanges::NOx::MIN_SPREAD);
    } else if (chart == ui_TempScreen_TempChart) {
        calculate_adaptive_range(chart, active_buffer, 
                               ChartRanges::Temperature::DEFAULT_MIN, 
                               ChartRanges::Temperature::DEFAULT_MAX, 
                               ChartRanges::Temperature::MIN_SPREAD);
    } else if (chart == ui_RHScreen_RHChart) {
        calculate_adaptive_range(chart, active_buffer, 
                               ChartRanges::Humidity::DEFAULT_MIN, 
                               ChartRanges::Humidity::DEFAULT_MAX, 
                               ChartRanges::Humidity::MIN_SPREAD);
    }

    // Update the chart series with the selected buffer
    lv_chart_set_ext_y_array(chart, series, active_buffer);
    if (chart == ui_PMScreen_PMChart && series2 && buffer2 && series3 && buffer3 && series4 && buffer4) {
        lv_chart_set_ext_y_array(chart, series2, active_buffer2);
        lv_chart_set_ext_y_array(chart, series3, active_buffer3);
        lv_chart_set_ext_y_array(chart, series4, active_buffer4);
    }
}

void DisplayTask::init_display() {
    // Initialize LVGL
    lv_init();

    // Initialize TFT display
    tft.begin();
    tft.setRotation(0); // Portrait orientation

    // Initialize LEDC PWM for backlight control
    ledcSetup(kLEDCChannel, kLEDCFrequency, kLEDCResolution);
    ledcAttachPin(PIN_LCD_BL, kLEDCChannel);
    // Start with full brightness
    ledcWrite(kLEDCChannel, 255);

    // Initialize display buffer
    lv_disp_draw_buf_init(&draw_buf, display_buffer, NULL, kBufferSize);

    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = kScreenWidth;
    disp_drv.ver_res = kScreenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize UI
    vTaskDelay(pdMS_TO_TICKS(5)); //yield to other tasks
    ui_init();
    vTaskDelay(pdMS_TO_TICKS(5)); //yield to other tasks

    // Read altitude from preferences and update UI
    Preferences prefs;
    if (prefs.begin(PREF_NAMESPACE, true)) {
        int32_t altitude = prefs.getInt(PREF_ALTITUDE_KEY, 0);
        lv_label_set_text_fmt(ui_AltitudeScreen_TargetValue, "%d", altitude);
        prefs.end();
    }

    // Hide settings screen containers
    // FRC Screen
    lv_label_set_text(ui_FRCScreen_LabelUp, "");
    lv_img_set_src(ui_FRCScreen_ImageUp, &ui_img_blank_png);
    lv_label_set_text(ui_FRCScreen_LabelDown, "");
    lv_img_set_src(ui_FRCScreen_ImageDown, &ui_img_blank_png);

    // Altitude Screen
    lv_label_set_text(ui_AltitudeScreen_LabelUp, "");
    lv_img_set_src(ui_AltitudeScreen_ImageUp, &ui_img_blank_png);
    lv_label_set_text(ui_AltitudeScreen_LabelDown, "");
    lv_img_set_src(ui_AltitudeScreen_ImageDown, &ui_img_blank_png);

    // Chart Time Screen
    lv_img_set_src(ui_ChartTimeScreen_ImageUp, &ui_img_blank_png);
    lv_img_set_src(ui_ChartTimeScreen_ImageDown, &ui_img_blank_png);

    // Brightness Screen
    lv_img_set_src(ui_BrightnessScreen_ImageUp, &ui_img_blank_png);
    lv_img_set_src(ui_BrightnessScreen_ImageDown, &ui_img_blank_png);

    // Initialize ring buffers
    init_buffers();

    // Configure antialiasing for all charts
    configure_chart_antialiasing(ui_PMScreen_PMChart);
    configure_chart_antialiasing(ui_CO2Screen_CO2Chart);
    configure_chart_antialiasing(ui_VOCScreen_VOCChart);
    configure_chart_antialiasing(ui_NOxScreen_NOxChart);
    configure_chart_antialiasing(ui_TempScreen_TempChart);
    configure_chart_antialiasing(ui_RHScreen_RHChart);

    // Initialize chart series with ring buffers
    // PM Chart - 4 series for different PM sizes
    pm1_series = lv_chart_add_series(ui_PMScreen_PMChart, lv_color_hex(0x3981F6), LV_CHART_AXIS_PRIMARY_Y);
    pm2p5_series = lv_chart_add_series(ui_PMScreen_PMChart, lv_color_hex(0xEE489C), LV_CHART_AXIS_PRIMARY_Y);
    pm4_series = lv_chart_add_series(ui_PMScreen_PMChart, lv_color_hex(0xAC55F6), LV_CHART_AXIS_PRIMARY_Y);
    pm10_series = lv_chart_add_series(ui_PMScreen_PMChart, lv_color_hex(0xFFFFFF), LV_CHART_AXIS_PRIMARY_Y);
    
    // Other charts - one series each
    co2_series = lv_chart_add_series(ui_CO2Screen_CO2Chart, lv_color_hex(0x3981F6), LV_CHART_AXIS_PRIMARY_Y);
    voc_series = lv_chart_add_series(ui_VOCScreen_VOCChart, lv_color_hex(0x3981F6), LV_CHART_AXIS_PRIMARY_Y);
    nox_series = lv_chart_add_series(ui_NOxScreen_NOxChart, lv_color_hex(0x3981F6), LV_CHART_AXIS_PRIMARY_Y);
    temp_series = lv_chart_add_series(ui_TempScreen_TempChart, lv_color_hex(0x3981F6), LV_CHART_AXIS_PRIMARY_Y);
    rh_series = lv_chart_add_series(ui_RHScreen_RHChart, lv_color_hex(0x3981F6), LV_CHART_AXIS_PRIMARY_Y);

    // Initialize range labels
    pm_min_label = ui_PMScreen_YMinValue;
    pm_max_label = ui_PMScreen_YMaxValue;
    co2_min_label = ui_CO2Screen_YMinValue;
    co2_max_label = ui_CO2Screen_YMaxValue;
    voc_min_label = ui_VOCScreen_YMinValue;
    voc_max_label = ui_VOCScreen_YMaxValue;
    nox_min_label = ui_NOxScreen_YMinValue;
    nox_max_label = ui_NOxScreen_YMaxValue;
    temp_min_label = ui_TempScreen_YMinValue;
    temp_max_label = ui_TempScreen_YMaxValue;
    rh_min_label = ui_RHScreen_YMinValue;
    rh_max_label = ui_RHScreen_YMaxValue;

    // Update all chart series with initial ring buffer values
    update_chart_series(ui_PMScreen_PMChart, pm1_series, &pm1_buffers,
                                pm2p5_series, &pm2p5_buffers,
                                pm4_series, &pm4_buffers,
                                pm10_series, &pm10_buffers);
    update_chart_series(ui_CO2Screen_CO2Chart, co2_series, &co2_buffers);
    update_chart_series(ui_VOCScreen_VOCChart, voc_series, &voc_buffers);
    update_chart_series(ui_NOxScreen_NOxChart, nox_series, &nox_buffers);
    update_chart_series(ui_TempScreen_TempChart, temp_series, &temp_buffers);
    update_chart_series(ui_RHScreen_RHChart, rh_series, &rh_buffers);

    // Set Chart Time Screen to Short Term
    currentDisplayMode = ChartDisplayMode::ShortTerm;
    lv_obj_clear_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);
    lv_obj_add_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
}

void DisplayTask::update_indicator_state(lv_obj_t* tile, IndicatorState state) {
    // Get the indicator object from the tile using the proper component selector
    lv_obj_t* indicator = ui_comp_get_child(tile, UI_COMP_TILE_INDICATOR);
    
    // Clear all states first
    lv_obj_clear_state(indicator, LV_STATE_USER_1);
    lv_obj_clear_state(indicator, LV_STATE_USER_2);
    lv_obj_clear_state(indicator, LV_STATE_USER_3);
    
    // Set the appropriate state
    switch (state) {
        case IndicatorState::Green:
            // Actively set default state
            lv_obj_clear_state(indicator, LV_STATE_USER_1 | LV_STATE_USER_2 | LV_STATE_USER_3);
            break;
        case IndicatorState::Orange:
            lv_obj_add_state(indicator, LV_STATE_USER_1);
            break;
        case IndicatorState::Red:
            lv_obj_add_state(indicator, LV_STATE_USER_2);
            break;
        case IndicatorState::Blue:
            lv_obj_add_state(indicator, LV_STATE_USER_3);
            break;
    }
}

void DisplayTask::update_all_indicators(const SensorData& data) {
    // Temperature thresholds
    if (data.temperature < SensorThresholds::Temperature::BLUE_MAX) {
        update_indicator_state(ui_MainScreen_TileT, IndicatorState::Blue);
    } else if (data.temperature <= SensorThresholds::Temperature::GREEN_MAX) {
        update_indicator_state(ui_MainScreen_TileT, IndicatorState::Green);
    } else {
        update_indicator_state(ui_MainScreen_TileT, IndicatorState::Red);
    }

    // Humidity thresholds
    if (data.humidity >= SensorThresholds::Humidity::GREEN_MIN && 
        data.humidity <= SensorThresholds::Humidity::GREEN_MAX) {
        update_indicator_state(ui_MainScreen_TileRH, IndicatorState::Green);
    } else if ((data.humidity >= SensorThresholds::Humidity::ORANGE_MIN && 
                data.humidity < SensorThresholds::Humidity::GREEN_MIN) || 
               (data.humidity > SensorThresholds::Humidity::GREEN_MAX && 
                data.humidity <= SensorThresholds::Humidity::ORANGE_MAX)) {
        update_indicator_state(ui_MainScreen_TileRH, IndicatorState::Orange);
    } else {
        update_indicator_state(ui_MainScreen_TileRH, IndicatorState::Red);
    }

    // CO2 thresholds
    if (data.co2 < SensorThresholds::CO2::BLUE_MAX) {
        update_indicator_state(ui_MainScreen_TileCO2, IndicatorState::Blue);
    } else if (data.co2 < SensorThresholds::CO2::GREEN_MAX) {
        update_indicator_state(ui_MainScreen_TileCO2, IndicatorState::Green);
    } else if (data.co2 <= SensorThresholds::CO2::ORANGE_MAX) {
        update_indicator_state(ui_MainScreen_TileCO2, IndicatorState::Orange);
    } else {
        update_indicator_state(ui_MainScreen_TileCO2, IndicatorState::Red);
    }

    // VOC thresholds
    if (data.vocIndex < SensorThresholds::VOC::BLUE_MAX) {
        update_indicator_state(ui_MainScreen_TileVOC, IndicatorState::Blue);
    } else if (data.vocIndex <= SensorThresholds::VOC::GREEN_MAX) {
        update_indicator_state(ui_MainScreen_TileVOC, IndicatorState::Green);
    } else if (data.vocIndex <= SensorThresholds::VOC::ORANGE_MAX) {
        update_indicator_state(ui_MainScreen_TileVOC, IndicatorState::Orange);
    } else {
        update_indicator_state(ui_MainScreen_TileVOC, IndicatorState::Red);
    }

    // NOx thresholds
    if (data.noxIndex <= SensorThresholds::NOx::GREEN_MAX) {
        update_indicator_state(ui_MainScreen_TileNOx, IndicatorState::Green);
    } else if (data.noxIndex <= SensorThresholds::NOx::ORANGE_MAX) {
        update_indicator_state(ui_MainScreen_TileNOx, IndicatorState::Orange);
    } else {
        update_indicator_state(ui_MainScreen_TileNOx, IndicatorState::Red);
    }

    // PM2.5 thresholds
    if (data.pm2p5 < SensorThresholds::PM25::GREEN_MAX) {
        update_indicator_state(ui_MainScreen_TilePM, IndicatorState::Green);
    } else if (data.pm2p5 <= SensorThresholds::PM25::ORANGE_MAX) {
        update_indicator_state(ui_MainScreen_TilePM, IndicatorState::Orange);
    } else {
        update_indicator_state(ui_MainScreen_TilePM, IndicatorState::Red);
    }
}

void DisplayTask::switchScreen(uint8_t screenIndex) {
    switch (screenIndex) {
        case 0:  // MainScreen
            lv_disp_load_scr(ui_MainScreen);
            break;
        case 1:  // PMScreen
            lv_disp_load_scr(ui_PMScreen);
            break;
        case 2:  // CO2Screen
            lv_disp_load_scr(ui_CO2Screen);
            break;
        case 3:  // VOCScreen
            lv_disp_load_scr(ui_VOCScreen);
            break;
        case 4:  // NOxScreen
            lv_disp_load_scr(ui_NOxScreen);
            break;
        case 5:  // TempScreen
            lv_disp_load_scr(ui_TempScreen);
            break;
        case 6:  // RHScreen
            lv_disp_load_scr(ui_RHScreen);
            break;
        case 7:  // SettingsScreen
            lv_disp_load_scr(ui_SettingsScreen);
            break;
        case 8:  // FRCScreen
            lv_disp_load_scr(ui_FRCScreen);
            break;
        case 9:  // AltitudeScreen
            lv_disp_load_scr(ui_AltitudeScreen);
            break;
        case 10: // ChartTimeScreen
            lv_disp_load_scr(ui_ChartTimeScreen);
            break;
        case 11: // BrightnessScreen
            lv_disp_load_scr(ui_BrightnessScreen);
            break;
        default:
            // If an invalid screen index is provided, default to MainScreen
            lv_disp_load_scr(ui_MainScreen);
            currentState = ScreenState::MainScreen;
            break;
    }
}

void DisplayTask::update_value_text(lv_obj_t* label, float value, uint8_t decimals) {
    char buffer[16];
    
    // Check for unknown values
    bool is_unknown = false;
    
    if (label == ui_PMScreen_Value || label == ui_PMScreen_Value1 || 
        label == ui_PMScreen_Value2 || label == ui_PMScreen_Value3) {
        // PM values: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::PM25::MIN || value > SensorThresholds::PM25::MAX);
    } else if (label == ui_CO2Screen_Value) {
        // CO2: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::CO2::MIN || value > SensorThresholds::CO2::MAX);
    } else if (label == ui_VOCScreen_Value) {
        // VOC Index: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::VOC::MIN || value > SensorThresholds::VOC::MAX);
    } else if (label == ui_NOxScreen_Value) {
        // NOx Index: Unknown if value is negative or exceeds MAX
        is_unknown = (value < SensorThresholds::NOx::MIN || value > SensorThresholds::NOx::MAX);
    } else if (label == ui_TempScreen_Value) {
        // Temperature: Unknown if value is outside MIN to MAX
        is_unknown = (value < SensorThresholds::Temperature::MIN || value > SensorThresholds::Temperature::MAX);
    } else if (label == ui_RHScreen_Value) {
        // Humidity: Unknown if value is outside MIN to MAX
        is_unknown = (value < SensorThresholds::Humidity::MIN || value > SensorThresholds::Humidity::MAX);
    }
    
    if (is_unknown) {
        lv_label_set_text(label, "-");
        return;
    }
    
    // Special formatting for different measurements
    if ((label == ui_PMScreen_Value || label == ui_PMScreen_Value1 || 
         label == ui_PMScreen_Value2 || label == ui_PMScreen_Value3) && value >= 100.0f) {
        // PM >= 100: show as integer
        snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
    } else {
        // Default: show with decimals decimal place
        snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    }
    
    lv_label_set_text(label, buffer);
    
    // Set color based on thresholds
    if (label == ui_PMScreen_Value || label == ui_PMScreen_Value1 || 
        label == ui_PMScreen_Value2 || label == ui_PMScreen_Value3) {
        // PM values
        if (value < SensorThresholds::PM25::GREEN_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Green);
        } else if (value <= SensorThresholds::PM25::ORANGE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Orange);
        } else {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Red);
        }
    } else if (label == ui_CO2Screen_Value) {
        // CO2
        if (value < SensorThresholds::CO2::BLUE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Blue);
        } else if (value < SensorThresholds::CO2::GREEN_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Green);
        } else if (value <= SensorThresholds::CO2::ORANGE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Orange);
        } else {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Red);
        }
    } else if (label == ui_VOCScreen_Value) {
        // VOC
        if (value < SensorThresholds::VOC::BLUE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Blue);
        } else if (value <= SensorThresholds::VOC::GREEN_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Green);
        } else if (value <= SensorThresholds::VOC::ORANGE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Orange);
        } else {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Red);
        }
    } else if (label == ui_NOxScreen_Value) {
        // NOx
        if (value <= SensorThresholds::NOx::GREEN_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Green);
        } else if (value <= SensorThresholds::NOx::ORANGE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Orange);
        } else {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Red);
        }
    } else if (label == ui_TempScreen_Value) {
        // Temperature
        if (value < SensorThresholds::Temperature::BLUE_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Blue);
        } else if (value <= SensorThresholds::Temperature::GREEN_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Green);
        } else {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Red);
        }
    } else if (label == ui_RHScreen_Value) {
        // Humidity
        if (value >= SensorThresholds::Humidity::GREEN_MIN && 
            value <= SensorThresholds::Humidity::GREEN_MAX) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Green);
        } else if ((value >= SensorThresholds::Humidity::ORANGE_MIN && 
                    value < SensorThresholds::Humidity::GREEN_MIN) || 
                   (value > SensorThresholds::Humidity::GREEN_MAX && 
                    value <= SensorThresholds::Humidity::ORANGE_MAX)) {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Orange);
        } else {
            ui_object_set_themeable_style_property(label, LV_PART_MAIN | LV_STATE_DEFAULT, LV_STYLE_TEXT_COLOR, _ui_theme_color_Red);
        }
    }
}

void DisplayTask::handleLeftButtonPress() {
    if (inSettingsMode) {
        switch (currentState) {
            case ScreenState::FRCScreen: {
                // Read current value and decrease by 10ppm
                const char* currentText = lv_label_get_text(ui_FRCScreen_TargetValue);
                int currentValue = atoi(currentText);
                if (currentValue > 400) {
                    currentValue -= 10;
                    lv_label_set_text_fmt(ui_FRCScreen_TargetValue, "%d", currentValue);
                }
                break;
            }
                
            case ScreenState::AltitudeScreen: {
                // Read current value and decrease by 25m
                const char* currentText = lv_label_get_text(ui_AltitudeScreen_TargetValue);
                int currentValue = atoi(currentText);
                if (currentValue > 0) {
                    currentValue -= 25;
                    lv_label_set_text_fmt(ui_AltitudeScreen_TargetValue, "%d", currentValue);
                }
                break;
            }
                
            case ScreenState::ChartTimeScreen:
                cycleChartDisplayMode();
                break;
                
            case ScreenState::BrightnessScreen:
                // Move checkbox selection right
                if (lv_obj_has_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
                    setDisplayBrightness(191); // 75% of 255
                } else if (lv_obj_has_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
                    setDisplayBrightness(128); // 50% of 255
                } else if (lv_obj_has_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
                    setDisplayBrightness(64); // 25%
                } else if (lv_obj_has_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
                    setDisplayBrightness(255); // 100%
                }
                break;
                
            default:
                break;
        }
        return;
    }

    switch (currentState) {
        case ScreenState::MainScreen:
            currentState = ScreenState::SettingsScreen;
            break;
        case ScreenState::PMScreen:
            currentState = ScreenState::MainScreen;
            break;
        case ScreenState::CO2Screen:
            currentState = ScreenState::PMScreen;
            break;
        case ScreenState::VOCScreen:
            currentState = ScreenState::CO2Screen;
            break;
        case ScreenState::NOxScreen:
            currentState = ScreenState::VOCScreen;
            break;
        case ScreenState::TempScreen:
            currentState = ScreenState::NOxScreen;
            break;
        case ScreenState::RHScreen:
            currentState = ScreenState::TempScreen;
            break;
        case ScreenState::SettingsScreen:
            currentState = ScreenState::RHScreen;
            break;
        case ScreenState::FRCScreen:
            currentState = ScreenState::BrightnessScreen;
            break;
        case ScreenState::AltitudeScreen:
            currentState = ScreenState::FRCScreen;
            break;
        case ScreenState::ChartTimeScreen:
            currentState = ScreenState::AltitudeScreen;
            break;
        case ScreenState::BrightnessScreen:
            currentState = ScreenState::ChartTimeScreen;
            break;
    }
    switchScreen(static_cast<uint8_t>(currentState));
}

void DisplayTask::handleRightButtonPress() {
    if (inSettingsMode) {
        switch (currentState) {
            case ScreenState::FRCScreen: {
                // Read current value and increase by 10ppm
                const char* currentText = lv_label_get_text(ui_FRCScreen_TargetValue);
                int currentValue = atoi(currentText);
                currentValue += 10;
                lv_label_set_text_fmt(ui_FRCScreen_TargetValue, "%d", currentValue);
                break;
            }
                
            case ScreenState::AltitudeScreen: {
                // Read current value and increase by 25m
                const char* currentText = lv_label_get_text(ui_AltitudeScreen_TargetValue);
                int currentValue = atoi(currentText);
                if (currentValue < 3000){
                    currentValue += 25;
                    lv_label_set_text_fmt(ui_AltitudeScreen_TargetValue, "%d", currentValue);
                }
                break;
            }
                
            case ScreenState::ChartTimeScreen:
                cycleChartDisplayMode(true);
                break;
                
            case ScreenState::BrightnessScreen:
                // Move checkbox selection left
                if (lv_obj_has_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
                    setDisplayBrightness(64); // 25%
                } else if (lv_obj_has_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
                    setDisplayBrightness(255); // 100%
                } else if (lv_obj_has_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
                    setDisplayBrightness(191); // 75%
                } else if (lv_obj_has_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED)) {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
                    setDisplayBrightness(128); // 50%
                }
                break;
                
            default:
                break;
        }
        return;
    }

    switch (currentState) {
        case ScreenState::MainScreen:
            currentState = ScreenState::PMScreen;
            break;
        case ScreenState::PMScreen:
            currentState = ScreenState::CO2Screen;
            break;
        case ScreenState::CO2Screen:
            currentState = ScreenState::VOCScreen;
            break;
        case ScreenState::VOCScreen:
            currentState = ScreenState::NOxScreen;
            break;
        case ScreenState::NOxScreen:
            currentState = ScreenState::TempScreen;
            break;
        case ScreenState::TempScreen:
            currentState = ScreenState::RHScreen;
            break;
        case ScreenState::RHScreen:
            currentState = ScreenState::SettingsScreen;
            break;
        case ScreenState::SettingsScreen:
            currentState = ScreenState::MainScreen;
            break;
        case ScreenState::FRCScreen:
            currentState = ScreenState::AltitudeScreen;
            break;
        case ScreenState::AltitudeScreen:
            currentState = ScreenState::ChartTimeScreen;
            break;
        case ScreenState::ChartTimeScreen:
            currentState = ScreenState::BrightnessScreen;
            break;
        case ScreenState::BrightnessScreen:
            currentState = ScreenState::FRCScreen;
            break;
    }
    switchScreen(static_cast<uint8_t>(currentState));
}

void DisplayTask::handleLeftButtonLongPress() {
    if (inSettingsMode) {
        // Restore saved state before exiting settings mode
        switch (currentState) {
            case ScreenState::FRCScreen: {
                // Restore saved FRC target value
                lv_label_set_text_fmt(ui_FRCScreen_TargetValue, "%d", savedFRCTargetValue);
                break;
            }
                
            case ScreenState::AltitudeScreen: {
                // Restore saved altitude value
                lv_label_set_text_fmt(ui_AltitudeScreen_TargetValue, "%d", savedAltitudeValue);
                break;
            }
                
            case ScreenState::ChartTimeScreen: {
                cycleChartDisplayMode(false, true);
                break;
            }
                
            case ScreenState::BrightnessScreen: {
                // Restore saved checkbox states
                if (savedBrightness100) {
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
                    setDisplayBrightness(255); // 100%
                } else {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
                }
                if (savedBrightness75) {
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
                    setDisplayBrightness(191); // 75%
                } else {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
                }
                if (savedBrightness50) {
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
                    setDisplayBrightness(128); // 50%
                } else {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
                }
                if (savedBrightness25) {
                    lv_obj_add_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
                    setDisplayBrightness(64); // 25%
                } else {
                    lv_obj_clear_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
                }
                break;
            }
                
            default:
                break;
        }
        
        exitSettingsMode();
        return;
    }

    // Only handle long press on settings-related screens
    switch (currentState) {
        case ScreenState::FRCScreen:
        case ScreenState::AltitudeScreen:
        case ScreenState::ChartTimeScreen:
        case ScreenState::BrightnessScreen:
            currentState = ScreenState::SettingsScreen;
            switchScreen(static_cast<uint8_t>(currentState));
            break;
        default:
            // Ignore long press on other screens
            break;
    }
}

void DisplayTask::handleRightButtonLongPress() {
    if (inSettingsMode) {
        exitSettingsMode();
        // Apply the settings when exiting settings mode
        switch (currentState) {
            case ScreenState::FRCScreen: {
                int32_t frcValue = atoi(lv_label_get_text(ui_FRCScreen_TargetValue));
                I2CScanTask::getInstance().setFRCValue(frcValue);
                break;
            }
            case ScreenState::AltitudeScreen: {
                int32_t altitude = atoi(lv_label_get_text(ui_AltitudeScreen_TargetValue));
                I2CScanTask::getInstance().setAltitude(altitude);
                break;
            }
        }
        return;
    }

    switch (currentState) {
        case ScreenState::PMScreen:
        case ScreenState::CO2Screen:
        case ScreenState::VOCScreen:
        case ScreenState::NOxScreen:
        case ScreenState::TempScreen:
        case ScreenState::RHScreen:
            cycleChartDisplayMode();
            break;
        case ScreenState::SettingsScreen:
            currentState = ScreenState::FRCScreen;
            switchScreen(static_cast<uint8_t>(currentState));
            break;
        case ScreenState::FRCScreen:
        case ScreenState::AltitudeScreen:
        case ScreenState::ChartTimeScreen:
        case ScreenState::BrightnessScreen:
            enterSettingsMode();
            break;
        default:
            // Ignore long press on other screens
            break;
    }
}

void DisplayTask::enterSettingsMode() {
    inSettingsMode = true;
    
    switch (currentState) {
        case ScreenState::FRCScreen: {
            // Save current FRC target value
            const char* currentText = lv_label_get_text(ui_FRCScreen_TargetValue);
            savedFRCTargetValue = atoi(currentText);
            
            lv_img_set_src(ui_FRCScreen_ImageUp, &ui_img_816914973);  // arrow-big-up.png
            lv_img_set_src(ui_FRCScreen_ImageDown, &ui_img_810620936);  // arrow-big-down.png
            lv_label_set_text(ui_FRCScreen_LabelUp, "+10ppm");
            lv_label_set_text(ui_FRCScreen_LabelDown, "-10ppm");
            lv_label_set_text(ui_FRCScreen_LabelRight, "long press to apply FRC");
            lv_label_set_text(ui_FRCScreen_LabelLeft, "long press to cancel");
            break;
        }
            
        case ScreenState::AltitudeScreen: {
            // Save current altitude value
            const char* currentText = lv_label_get_text(ui_AltitudeScreen_TargetValue);
            savedAltitudeValue = atoi(currentText);
            
            lv_img_set_src(ui_AltitudeScreen_ImageUp, &ui_img_816914973);  // arrow-big-up.png
            lv_img_set_src(ui_AltitudeScreen_ImageDown, &ui_img_810620936);  // arrow-big-down.png
            lv_label_set_text(ui_AltitudeScreen_LabelUp, "+25m");
            lv_label_set_text(ui_AltitudeScreen_LabelDown, "-25m");
            lv_label_set_text(ui_AltitudeScreen_LabelRight, "long press to apply altitude");
            lv_label_set_text(ui_AltitudeScreen_LabelLeft, "long press to cancel");
            break;
        }
            
        case ScreenState::ChartTimeScreen: {
            // Save current checkbox states
            savedChartDisplayMode = currentDisplayMode;
            
            lv_img_set_src(ui_ChartTimeScreen_ImageUp, &ui_img_816914973);  // arrow-big-up.png
            lv_img_set_src(ui_ChartTimeScreen_ImageDown, &ui_img_810620936);  // arrow-big-down.png
            lv_label_set_text(ui_ChartTimeScreen_LabelRight, "long press to apply setting");
            lv_label_set_text(ui_ChartTimeScreen_LabelLeft, "long press to cancel");
            break;
        }
            
        case ScreenState::BrightnessScreen: {
            // Save current checkbox states
            savedBrightness100 = lv_obj_has_state(ui_BrightnessScreen_Checkbox100, LV_STATE_CHECKED);
            savedBrightness75 = lv_obj_has_state(ui_BrightnessScreen_Checkbox75, LV_STATE_CHECKED);
            savedBrightness50 = lv_obj_has_state(ui_BrightnessScreen_Checkbox50, LV_STATE_CHECKED);
            savedBrightness25 = lv_obj_has_state(ui_BrightnessScreen_Checkbox25, LV_STATE_CHECKED);
            
            lv_img_set_src(ui_BrightnessScreen_ImageUp, &ui_img_816914973);  // arrow-big-up.png
            lv_img_set_src(ui_BrightnessScreen_ImageDown, &ui_img_810620936);  // arrow-big-down.png
            lv_label_set_text(ui_BrightnessScreen_LabelRight, "long press to apply setting");
            lv_label_set_text(ui_BrightnessScreen_LabelLeft, "long press to cancel");
            break;
        }
            
        default:
            break;
    }
}

void DisplayTask::exitSettingsMode() {
    inSettingsMode = false;
    
    switch (currentState) {
        case ScreenState::FRCScreen:
            lv_label_set_text(ui_FRCScreen_LabelUp, "");
            lv_img_set_src(ui_FRCScreen_ImageUp, &ui_img_blank_png);
            lv_label_set_text(ui_FRCScreen_LabelDown, "");
            lv_img_set_src(ui_FRCScreen_ImageDown, &ui_img_blank_png);
            lv_label_set_text(ui_FRCScreen_LabelRight, "long press to adjust value");
            lv_label_set_text(ui_FRCScreen_LabelLeft, "long press to exit settings");
            break;
            
        case ScreenState::AltitudeScreen:
            lv_label_set_text(ui_AltitudeScreen_LabelUp, "");
            lv_img_set_src(ui_AltitudeScreen_ImageUp, &ui_img_blank_png);
            lv_label_set_text(ui_AltitudeScreen_LabelDown, "");
            lv_img_set_src(ui_AltitudeScreen_ImageDown, &ui_img_blank_png);
            lv_label_set_text(ui_AltitudeScreen_LabelRight, "long press to adjust value");
            lv_label_set_text(ui_AltitudeScreen_LabelLeft, "long press to exit settings");
            break;
            
        case ScreenState::ChartTimeScreen:
            lv_img_set_src(ui_ChartTimeScreen_ImageUp, &ui_img_blank_png);
            lv_img_set_src(ui_ChartTimeScreen_ImageDown, &ui_img_blank_png);
            lv_label_set_text(ui_ChartTimeScreen_LabelRight, "long press to adjust value");
            lv_label_set_text(ui_ChartTimeScreen_LabelLeft, "long press to exit settings");
            break;
            
        case ScreenState::BrightnessScreen:
            lv_img_set_src(ui_BrightnessScreen_ImageUp, &ui_img_blank_png);
            lv_img_set_src(ui_BrightnessScreen_ImageDown, &ui_img_blank_png);
            lv_label_set_text(ui_BrightnessScreen_LabelRight, "long press to adjust value");
            lv_label_set_text(ui_BrightnessScreen_LabelLeft, "long press to exit settings");
            break;
            
        default:
            break;
    }
}

void DisplayTask::updateParameterBuffer(ParameterBuffers* buffers, float value) {
    // Update short-term buffer (150 points, 1 point per second)
    update_ring_buffer(buffers->short_term_ring_buffer, value);
    
    // Accumulate 24 seconds average
    float scaled_value = valid_value(buffers->short_term_ring_buffer, value);
    if (scaled_value != -1.0f){
        buffers->mid_term_sum += scaled_value;
        buffers->mid_term_count++;
    }
    if (buffers->mid_term_count == kMidTermBufferSize - 1) {
        update_ring_buffer(buffers->mid_term_ring_buffer, buffers->mid_term_sum / kMidTermBufferSize);
        // Accumulate 576 seconds average
        buffers->long_term_sum += buffers->mid_term_sum / kMidTermBufferSize;
        buffers->long_term_count++;
        if (buffers->long_term_count == kLongTermBufferSize - 1) {
            update_ring_buffer(buffers->long_term_ring_buffer, buffers->long_term_sum / kLongTermBufferSize);
            buffers->long_term_sum = 0.0f;
            buffers->long_term_count = 0;
            }
        buffers->mid_term_sum = 0.0f;
        buffers->mid_term_count = 0;
    }
}

void DisplayTask::cycleChartDisplayMode(bool up, bool reset) {
    if (reset) {
        currentDisplayMode = savedChartDisplayMode;
        lv_obj_clear_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
        lv_obj_clear_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
        lv_obj_clear_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);

        // Restore saved checkbox states
        if (savedChartDisplayMode == ChartDisplayMode::ShortTerm) {
            lv_obj_add_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
        } else if (savedChartDisplayMode == ChartDisplayMode::MidTerm) {
            lv_obj_add_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
        } else if (savedChartDisplayMode == ChartDisplayMode::LongTerm) {
            lv_obj_add_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);
        }

    } else {
        if (up) {
            // Move checkbox selection up
            if (currentDisplayMode == ChartDisplayMode::ShortTerm) {
                lv_obj_clear_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
                lv_obj_add_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);
                currentDisplayMode = ChartDisplayMode::LongTerm;
            } else if (currentDisplayMode == ChartDisplayMode::MidTerm) {
                lv_obj_clear_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
                lv_obj_add_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
                currentDisplayMode = ChartDisplayMode::ShortTerm;
            } else if (currentDisplayMode == ChartDisplayMode::LongTerm) {
                lv_obj_clear_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);
                lv_obj_add_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
                currentDisplayMode = ChartDisplayMode::MidTerm;
            }
        } else {
            // Move checkbox selection down
            if (currentDisplayMode == ChartDisplayMode::ShortTerm) {
                lv_obj_clear_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
                lv_obj_add_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
                currentDisplayMode = ChartDisplayMode::MidTerm;
            } else if (currentDisplayMode == ChartDisplayMode::MidTerm) {
                lv_obj_clear_state(ui_ChartTimeScreen_CheckboxMedium, LV_STATE_CHECKED);
                lv_obj_add_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);
                currentDisplayMode = ChartDisplayMode::LongTerm;
            } else if (currentDisplayMode == ChartDisplayMode::LongTerm) {
                lv_obj_clear_state(ui_ChartTimeScreen_CheckboxLong, LV_STATE_CHECKED);
                lv_obj_add_state(ui_ChartTimeScreen_CheckboxShort, LV_STATE_CHECKED);
                currentDisplayMode = ChartDisplayMode::ShortTerm;
            }
        }
    }

    switch (currentDisplayMode) {
        case ChartDisplayMode::ShortTerm:
            // Update X-axis labels for all charts to show -2min30
            lv_label_set_text(ui_PMScreen_XMinValue, "-2min30");
            lv_label_set_text(ui_CO2Screen_XMinValue, "-2min30");
            lv_label_set_text(ui_VOCScreen_XMinValue, "-2min30");
            lv_label_set_text(ui_NOxScreen_XMinValue, "-2min30");
            lv_label_set_text(ui_TempScreen_XMinValue, "-2min30");
            lv_label_set_text(ui_RHScreen_XMinValue, "-2min30");
            break;
        case ChartDisplayMode::MidTerm:
            // Update X-axis labels for all charts to show -60min
            lv_label_set_text(ui_PMScreen_XMinValue, "-60min");
            lv_label_set_text(ui_CO2Screen_XMinValue, "-60min");
            lv_label_set_text(ui_VOCScreen_XMinValue, "-60min");
            lv_label_set_text(ui_NOxScreen_XMinValue, "-60min");
            lv_label_set_text(ui_TempScreen_XMinValue, "-60min");
            lv_label_set_text(ui_RHScreen_XMinValue, "-60min");
            break;
        case ChartDisplayMode::LongTerm:
            // Update X-axis labels for all charts to show -24h
            lv_label_set_text(ui_PMScreen_XMinValue, "-24h");
            lv_label_set_text(ui_CO2Screen_XMinValue, "-24h");
            lv_label_set_text(ui_VOCScreen_XMinValue, "-24h");
            lv_label_set_text(ui_NOxScreen_XMinValue, "-24h");
            lv_label_set_text(ui_TempScreen_XMinValue, "-24h");
            lv_label_set_text(ui_RHScreen_XMinValue, "-24h");
            break;
    }
    
    // Update all chart series to reflect the new display mode
    update_chart_series(ui_PMScreen_PMChart, pm1_series, &pm1_buffers,
                       pm2p5_series, &pm2p5_buffers,
                       pm4_series, &pm4_buffers,
                       pm10_series, &pm10_buffers);
    
    update_chart_series(ui_CO2Screen_CO2Chart, co2_series, &co2_buffers);
    update_chart_series(ui_VOCScreen_VOCChart, voc_series, &voc_buffers);
    update_chart_series(ui_NOxScreen_NOxChart, nox_series, &nox_buffers);
    update_chart_series(ui_TempScreen_TempChart, temp_series, &temp_buffers);
    update_chart_series(ui_RHScreen_RHChart, rh_series, &rh_buffers);
}

void DisplayTask::setDisplayBrightness(uint8_t brightness) {
    #ifdef DEBUG_MODE
    // Check current duty cycle
    uint32_t currentDuty = ledcRead(kLEDCChannel);
    Serial.printf("Current duty cycle: %u, Setting to: %u\n", currentDuty, brightness);
    #endif
    
    ledcWrite(kLEDCChannel, brightness);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    #ifdef DEBUG_MODE
    currentDuty = ledcRead(kLEDCChannel);
    Serial.printf("Final PWM duty cycle: %u\n", currentDuty);
    #endif
}
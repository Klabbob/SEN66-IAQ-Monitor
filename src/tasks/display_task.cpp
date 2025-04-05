#include "tasks/display_task.h"
#include "tasks/live_data_manager.h"
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
        // Handle LVGL tasks
        lv_task_handler();
        
        // Check for new sensor data
        if (xQueueReceive(xDataQueue, &message, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) == pdTRUE) {
            const SensorData& data = message.data;
            
            // Update all tiles with new values
            instance.update_tile_value(ui_MainScreen_TilePM, data.pm2p5);    // PM2.5
            instance.update_tile_value(ui_MainScreen_TileRH, data.humidity); // Relative Humidity
            instance.update_tile_value(ui_MainScreen_TileT, data.temperature); // Temperature
            instance.update_tile_value(ui_MainScreen_TileNOx, data.noxIndex); // NOx Index
            instance.update_tile_value(ui_MainScreen_TileVOC, data.vocIndex); // VOC Index
            instance.update_tile_value(ui_MainScreen_TileCO2, data.co2);     // CO2
        }
        
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

void DisplayTask::init_display() {
    // Initialize LVGL
    lv_init();

    // Initialize TFT display
    tft.begin();
    tft.setRotation(0); // Portrait orientation

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
    ui_init();

    // Set initial PM2.5 value
    update_tile_value(ui_MainScreen_TilePM, 25.5f);
}

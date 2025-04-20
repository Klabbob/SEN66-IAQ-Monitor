#include "tasks/button_handler.h"

// Initialize static member
TaskHandle_t ButtonHandler::xButtonTaskHandle = nullptr;

ButtonHandler& ButtonHandler::getInstance() {
    static ButtonHandler instance;
    return instance;
}

ButtonHandler::ButtonHandler() 
    : buttonLeft(PIN_BUTTON_LEFT)
    , buttonRight(PIN_BUTTON_RIGHT) {
    
    // Configure buttons
    buttonLeft.setPressedHandler([this](Button2& btn) { handleLeftButton(btn); });
    buttonRight.setPressedHandler([this](Button2& btn) { handleRightButton(btn); });
    
    // Set debounce time
    buttonLeft.setDebounceTime(50);
    buttonRight.setDebounceTime(50);
}

void ButtonHandler::buttonTask(void* parameter) {
    ButtonHandler& handler = getInstance();
    
    while (true) {
        handler.buttonLeft.loop();
        handler.buttonRight.loop();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
}

void ButtonHandler::handleLeftButton(Button2& btn) {
    DisplayTask::previousScreen();
}

void ButtonHandler::handleRightButton(Button2& btn) {
    DisplayTask::nextScreen();
} 
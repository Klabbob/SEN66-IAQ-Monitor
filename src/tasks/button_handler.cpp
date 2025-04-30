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
    buttonLeft.setClickHandler([this](Button2& btn) { handleLeftButton(btn); });
    buttonRight.setClickHandler([this](Button2& btn) { handleRightButton(btn); });
    buttonLeft.setLongClickDetectedHandler([this](Button2& btn) { handleLeftButtonLongPress(btn); });
    buttonRight.setLongClickDetectedHandler([this](Button2& btn) { handleRightButtonLongPress(btn); });
    
    // Set debounce time and long press duration
    buttonLeft.setDebounceTime(40);
    buttonRight.setDebounceTime(40);
    buttonLeft.setLongClickTime(500);  // 500ms for long press
    buttonRight.setLongClickTime(500);
    
    // Disable double-click detection
    buttonLeft.setDoubleClickTime(0);
    buttonRight.setDoubleClickTime(0);

    // Set long press as not retriggerable
    buttonLeft.setLongClickDetectedRetriggerable(false);
    buttonRight.setLongClickDetectedRetriggerable(false);
}

void ButtonHandler::buttonTask(void* parameter) {
    ButtonHandler& handler = getInstance();
    
    while (true) {
        handler.buttonLeft.loop();
        handler.buttonRight.loop();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void ButtonHandler::handleLeftButton(Button2& btn) {
    DisplayTask::getInstance().handleLeftButtonPress();
}

void ButtonHandler::handleRightButton(Button2& btn) {
    DisplayTask::getInstance().handleRightButtonPress();
}

void ButtonHandler::handleLeftButtonLongPress(Button2& btn) {
    DisplayTask::getInstance().handleLeftButtonLongPress();
}

void ButtonHandler::handleRightButtonLongPress(Button2& btn) {
    DisplayTask::getInstance().handleRightButtonLongPress();
}
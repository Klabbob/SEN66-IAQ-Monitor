#pragma once

#include <Button2.h>
#include <FreeRTOS.h>
#include <task.h>
#include "definitions.h"
#include "tasks/display_task.h"

class ButtonHandler {
public:
    static ButtonHandler& getInstance();
    static void buttonTask(void* parameter);
    static TaskHandle_t xButtonTaskHandle;

private:
    ButtonHandler();
    ~ButtonHandler() = default;
    
    // Delete copy constructor and assignment operator
    ButtonHandler(const ButtonHandler&) = delete;
    ButtonHandler& operator=(const ButtonHandler&) = delete;

    // Button objects
    Button2 buttonLeft;
    Button2 buttonRight;

    // Button callback functions
    void handleLeftButton(Button2& btn);
    void handleRightButton(Button2& btn);
}; 
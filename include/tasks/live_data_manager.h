#pragma once

#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "definitions.h"

// Configuration
#define MAX_SUBSCRIBERS 5
#define QUEUE_SIZE 10
#define QUEUE_TIMEOUT_MS 100

// Queue message structure
struct QueueMessage {
    SensorData data;
    uint32_t timestamp;
};

// Subscription structure
struct Subscription {
    TaskHandle_t subscriber;
    QueueHandle_t queue;
    bool active;
};

class LiveDataManager {
public:
    static LiveDataManager& getInstance();
    
    // Task function
    static void liveDataManagerTask(void* parameter);
    
    // Public interface
    bool subscribe(TaskHandle_t subscriber, QueueHandle_t* queue);
    void unsubscribe(TaskHandle_t subscriber);
    bool publish(const SensorData& data);
    
    // Task handle
    static TaskHandle_t xLiveDataManagerTaskHandle;

private:
    LiveDataManager() = default;
    ~LiveDataManager() = default;
    LiveDataManager(const LiveDataManager&) = delete;
    LiveDataManager& operator=(const LiveDataManager&) = delete;

    // Queue handles
    QueueHandle_t _ingressQueue;
    Subscription _subscriptions[MAX_SUBSCRIBERS];
    
    // Helper methods
    bool addSubscription(TaskHandle_t subscriber, QueueHandle_t queue);
    bool removeSubscription(TaskHandle_t subscriber);
    void forwardData(const QueueMessage& message);
    void handleQueueError(const char* operation);
}; 
#include "tasks/live_data_manager.h"

// Static member initialization
TaskHandle_t LiveDataManager::xLiveDataManagerTaskHandle = nullptr;

LiveDataManager& LiveDataManager::getInstance() {
    static LiveDataManager instance;
    return instance;
}

void LiveDataManager::liveDataManagerTask(void* parameter) {
    LiveDataManager& manager = getInstance();
    
    // Create ingress queue
    manager._ingressQueue = xQueueCreate(QUEUE_SIZE, sizeof(QueueMessage));
    if (manager._ingressQueue == nullptr) {
        Serial.println("Failed to create ingress queue!");
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize subscriptions
    for (auto& sub : manager._subscriptions) {
        sub.active = false;
    }
    
    QueueMessage message;
    while (true) {
        // Wait for new data
        if (xQueueReceive(manager._ingressQueue, &message, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) == pdTRUE) {
            // Forward data to all active subscribers
            manager.forwardData(message);
        }
        
        // Small delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool LiveDataManager::subscribe(TaskHandle_t subscriber, QueueHandle_t* queue) {
    // Create queue for subscriber
    *queue = xQueueCreate(QUEUE_SIZE, sizeof(QueueMessage));
    if (*queue == nullptr) {
        Serial.println("Failed to create subscriber queue!");
        return false;
    }
    
    // Add subscription
    return addSubscription(subscriber, *queue);
}

void LiveDataManager::unsubscribe(TaskHandle_t subscriber) {
    removeSubscription(subscriber);
}

bool LiveDataManager::publish(const SensorData& data) {
    QueueMessage message;
    message.data = data;
    message.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (xQueueSend(_ingressQueue, &message, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdTRUE) {
        handleQueueError("publish");
        return false;
    }
    
    return true;
}

bool LiveDataManager::addSubscription(TaskHandle_t subscriber, QueueHandle_t queue) {
    for (auto& sub : _subscriptions) {
        if (!sub.active) {
            sub.subscriber = subscriber;
            sub.queue = queue;
            sub.active = true;
            return true;
        }
    }
    
    Serial.println("No free subscription slots available!");
    return false;
}

bool LiveDataManager::removeSubscription(TaskHandle_t subscriber) {
    for (auto& sub : _subscriptions) {
        if (sub.active && sub.subscriber == subscriber) {
            if (sub.queue != nullptr) {
                vQueueDelete(sub.queue);
            }
            sub.active = false;
            return true;
        }
    }
    return false;
}

void LiveDataManager::forwardData(const QueueMessage& message) {
    for (const auto& sub : _subscriptions) {
        if (sub.active && sub.queue != nullptr) {
            if (xQueueSend(sub.queue, &message, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdTRUE) {
                handleQueueError("forward");
            }
        }
    }
}

void LiveDataManager::handleQueueError(const char* operation) {
    #ifdef DEBUG_MODE
    Serial.print("Queue error during ");
    Serial.println(operation);
    #endif
} 
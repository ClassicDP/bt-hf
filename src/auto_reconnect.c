#include "auto_reconnect.h"
#include "paired_devices.h"
#include "gap_handler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_hf_ag_api.h"
#include <string.h>

static const char* TAG = "AUTO_RECONNECT";

static esp_timer_handle_t reconnect_timer = NULL;
static auto_reconnect_state_t current_state = AUTO_RECONNECT_STATE_IDLE;
static int reconnect_attempts = 0;
static esp_bd_addr_t last_connected_device = {0};

// Внутренние функции
static void auto_reconnect_timer_callback(void* arg);
static void auto_reconnect_start_timer(void);
static void auto_reconnect_stop_timer(void);

esp_err_t auto_reconnect_init(void) {
    ESP_LOGI(TAG, "Initializing auto-reconnect module");
    
    // Создаем таймер для автоматического переподключения
    const esp_timer_create_args_t timer_args = {
        .callback = auto_reconnect_timer_callback,
        .name = "auto_reconnect_timer"
    };
    
    esp_err_t ret = esp_timer_create(&timer_args, &reconnect_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create auto-reconnect timer: %s", esp_err_to_name(ret));
        return ret;
    }
    
    current_state = AUTO_RECONNECT_STATE_IDLE;
    reconnect_attempts = 0;
    
    ESP_LOGI(TAG, "Auto-reconnect module initialized successfully");
    return ESP_OK;
}

void auto_reconnect_start(void) {
    if (current_state != AUTO_RECONNECT_STATE_IDLE) {
        ESP_LOGW(TAG, "Auto-reconnect already in progress, state: %d", current_state);
        return;
    }
    
    ESP_LOGI(TAG, "Starting auto-reconnect process");
    current_state = AUTO_RECONNECT_STATE_SEARCHING;
    reconnect_attempts = 0;
    
    // Получаем последнее подключенное устройство
    if (paired_devices_get_last_connected(last_connected_device) == ESP_OK) {
        ESP_LOGI(TAG, "Found last connected device: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(last_connected_device));
        auto_reconnect_start_timer();
    } else {
        ESP_LOGI(TAG, "No last connected device found");
        current_state = AUTO_RECONNECT_STATE_IDLE;
    }
}

void auto_reconnect_stop(void) {
    ESP_LOGI(TAG, "Stopping auto-reconnect process");
    auto_reconnect_stop_timer();
    current_state = AUTO_RECONNECT_STATE_IDLE;
    reconnect_attempts = 0;
}

void auto_reconnect_notify_connection_state(bool connected) {
    ESP_LOGI(TAG, "Connection state changed: %s", connected ? "connected" : "disconnected");
    
    if (connected) {
        // Подключение установлено
        auto_reconnect_stop_timer();
        current_state = AUTO_RECONNECT_STATE_CONNECTED;
        reconnect_attempts = 0;
    } else {
        // Подключение потеряно
        current_state = AUTO_RECONNECT_STATE_IDLE;
        
        // Запускаем переподключение через некоторое время
        auto_reconnect_start_timer();
    }
}

void auto_reconnect_notify_discovery_complete(void) {
    if (current_state != AUTO_RECONNECT_STATE_SEARCHING) {
        return;
    }
    
    ESP_LOGI(TAG, "Discovery complete, trying to connect to last device");
    current_state = AUTO_RECONNECT_STATE_CONNECTING;
    
    // Пытаемся подключиться к последнему устройству
    esp_err_t ret = esp_hf_ag_slc_connect(last_connected_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to device: %s", esp_err_to_name(ret));
        current_state = AUTO_RECONNECT_STATE_FAILED;
        auto_reconnect_start_timer(); // Попробуем снова
    }
}

void auto_reconnect_notify_connection_failed(void) {
    if (current_state != AUTO_RECONNECT_STATE_CONNECTING) {
        return;
    }
    
    ESP_LOGW(TAG, "Connection failed, attempt %d/%d", reconnect_attempts + 1, AUTO_RECONNECT_MAX_ATTEMPTS);
    
    reconnect_attempts++;
    if (reconnect_attempts < AUTO_RECONNECT_MAX_ATTEMPTS) {
        current_state = AUTO_RECONNECT_STATE_IDLE;
        auto_reconnect_start_timer();
    } else {
        ESP_LOGE(TAG, "Max reconnection attempts reached, giving up");
        current_state = AUTO_RECONNECT_STATE_FAILED;
        reconnect_attempts = 0;
    }
}

void auto_reconnect_notify_device_found(const esp_bd_addr_t bd_addr, const char* name, uint32_t cod) {
    // Заглушка для обработки найденного устройства
    ESP_LOGD(TAG, "Device found: %s", name);
}

auto_reconnect_state_t auto_reconnect_get_state(void) {
    return current_state;
}

bool auto_reconnect_is_active(void) {
    return (current_state != AUTO_RECONNECT_STATE_IDLE && current_state != AUTO_RECONNECT_STATE_CONNECTED);
}

static void auto_reconnect_timer_callback(void* arg) {
    ESP_LOGI(TAG, "Auto-reconnect timer fired, state: %d", current_state);
    
    if (current_state == AUTO_RECONNECT_STATE_IDLE) {
        // Начинаем поиск устройств
        current_state = AUTO_RECONNECT_STATE_SEARCHING;
        gap_start_discovery();
    } else if (current_state == AUTO_RECONNECT_STATE_FAILED) {
        // Попробуем снова
        current_state = AUTO_RECONNECT_STATE_IDLE;
        auto_reconnect_start_timer();
    }
}

static void auto_reconnect_start_timer(void) {
    if (reconnect_timer == NULL) {
        ESP_LOGE(TAG, "Reconnect timer not initialized");
        return;
    }
    
    esp_err_t ret = esp_timer_start_once(reconnect_timer, AUTO_RECONNECT_INTERVAL_MS * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start auto-reconnect timer: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Auto-reconnect timer started, will fire in %d ms", AUTO_RECONNECT_INTERVAL_MS);
    }
}

static void auto_reconnect_stop_timer(void) {
    if (reconnect_timer == NULL) {
        return;
    }
    
    esp_err_t ret = esp_timer_stop(reconnect_timer);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to stop auto-reconnect timer: %s", esp_err_to_name(ret));
    }
}

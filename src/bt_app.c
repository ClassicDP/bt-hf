#include "bt_app.h"
#include "gap_handler.h"
#include "hf_handler.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BT_APP";

void bt_app_init(void) {
    ESP_LOGI(TAG, "Initializing Bluetooth stack...");

    // NVS init - в соответствии с официальным примером
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_LOGI(TAG, "NVS initialized successfully");

    // Release memory for BLE if not used - как в официальном примере
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s release controller memory failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "BLE memory released successfully");

    // Initialize BT controller - согласно официальному примеру
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    // Явно устанавливаем режим Classic BT
    bt_cfg.mode = ESP_BT_MODE_CLASSIC_BT;
    
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Bluetooth controller initialized successfully");
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Bluetooth controller enabled successfully");

    // Initialize bluedroid - как в официальном примере
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    // Set device name - как в официальном примере  
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name("ESP32-HF-AG"));

    // Register GAP callback first
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));

    // Register HF AG callback - как в официальном примере
    ESP_ERROR_CHECK(esp_hf_ag_register_callback(hf_ag_event_handler));

    // Initialize HF AG - как в официальном примере
    ESP_ERROR_CHECK(esp_hf_ag_init());

    // Configure security - как в официальном примере
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '0';
    pin_code[1] = '0';
    pin_code[2] = '0';
    pin_code[3] = '0';
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(pin_type, 4, pin_code));

    // Set discoverable and connectable mode - как в официальном примере
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));

    ESP_LOGI(TAG, "✅ Bluetooth stack initialized successfully");
}

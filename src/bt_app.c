#include "bt_app.h"
#include "gap_handler.h"
#include "hf_handler.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include <string.h>

void bt_app_init(void) {
    // NVS init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // BT stack init
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Set name and register AG
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name("ESP32-AG"));
    ESP_ERROR_CHECK(esp_hf_ag_register_callback(hf_ag_event_handler));
    ESP_ERROR_CHECK(esp_hf_ag_init());

    // Set GAP callback
    esp_err_t err;
    err = esp_bt_gap_register_callback(gap_callback);
    if (err != ESP_OK) {
        ESP_LOGE("HF_AG_CONNECT", "Failed to register GAP callback: %s", esp_err_to_name(err));
    }
    err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    if (err != ESP_OK) {
        ESP_LOGE("HF_AG_CONNECT", "Failed to set scan mode: %s", esp_err_to_name(err));
    }

    // Start discovery (10s)
    ESP_LOGI("HF_AG_CONNECT", "🔍 Starting device discovery...");
    err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    if (err != ESP_OK) {
        ESP_LOGE("HF_AG_CONNECT", "Failed to start discovery: %s", esp_err_to_name(err));
    }
}

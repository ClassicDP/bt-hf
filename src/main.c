#include "bt_app.h"
#include "bt_app_core.h"
#include "gap_handler.h"
#include "hf_handler.h"
#include "console_handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt_device.h"

#define TARGET_NAME "OpenMove by AfterShokz"
#define BT_HF_AG_TAG "HF_AG_DEMO_MAIN"

// Преобразование адреса в строку
static char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }
    
    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

// Обработчик события инициализации стека
enum {
    BT_APP_EVT_STACK_UP = 0,
};

static void bt_hf_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_HF_AG_TAG, "%s evt %d", __func__, event);
    switch (event) {
        case BT_APP_EVT_STACK_UP:
            ESP_LOGI(BT_HF_AG_TAG, "Stack up event received");
            // Устанавливаем имя цели и запускаем обнаружение
            gap_set_target_name(TARGET_NAME);
            
            // Даем время системе Bluetooth полностью инициализироваться
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            // Сначала пробуем переподключиться к последнему устройству
            gap_try_reconnect_to_last_device();
            break;
            
        default:
            ESP_LOGE(BT_HF_AG_TAG, "%s unhandled evt %d", __func__, event);
            break;
    }
}

void app_main(void) {
    char bda_str[18] = {0};
    
    // Инициализация Bluetooth стека
    bt_app_init();
    
    // Создание задачи приложения
    bt_app_task_start_up();
    
    // Получение собственного адреса устройства
    ESP_LOGI(BT_HF_AG_TAG, "Own address:[%s]", 
             bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
    
    // Отправка события инициализации стека
    bt_app_work_dispatch(bt_hf_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);
    
    // Инициализация консольного обработчика
    console_handler_init();
    
    ESP_LOGI(BT_HF_AG_TAG, "=== HF AG Demo Started ===");
    ESP_LOGI(BT_HF_AG_TAG, "Target device: %s", TARGET_NAME);
    ESP_LOGI(BT_HF_AG_TAG, "Waiting for connection...");
}
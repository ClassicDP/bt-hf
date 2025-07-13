#include "paired_devices.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <time.h>

static const char *TAG = "PAIRED_DEVICES";
static const char *NVS_NAMESPACE = "paired_dev";
static const char *NVS_KEY_COUNT = "count";
static const char *NVS_KEY_DEVICE_PREFIX = "dev_";

static paired_device_t paired_devices[MAX_PAIRED_DEVICES];
static int paired_device_count = 0;
static nvs_handle_t nvs_handle_storage;

// Вспомогательная функция для получения строкового представления MAC адреса
static void bd_addr_to_string(const esp_bd_addr_t bd_addr, char *str) {
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
}

// Вспомогательная функция для сравнения MAC адресов
static bool bd_addr_equal(const esp_bd_addr_t addr1, const esp_bd_addr_t addr2) {
    return memcmp(addr1, addr2, ESP_BD_ADDR_LEN) == 0;
}

// Загрузка устройств из NVS
static esp_err_t load_devices_from_nvs(void) {
    size_t count_size = sizeof(paired_device_count);
    esp_err_t err = nvs_get_blob(nvs_handle_storage, NVS_KEY_COUNT, &paired_device_count, &count_size);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No paired devices found in NVS");
        paired_device_count = 0;
        return ESP_OK;
    }

    if (paired_device_count > MAX_PAIRED_DEVICES) {
        ESP_LOGW(TAG, "Too many devices in NVS (%d), limiting to %d", paired_device_count, MAX_PAIRED_DEVICES);
        paired_device_count = MAX_PAIRED_DEVICES;
    }

    // Загружаем каждое устройство
    for (int i = 0; i < paired_device_count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_DEVICE_PREFIX, i);
        
        size_t device_size = sizeof(paired_device_t);
        err = nvs_get_blob(nvs_handle_storage, key, &paired_devices[i], &device_size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load device %d from NVS: %s", i, esp_err_to_name(err));
            continue;
        }

        char addr_str[18];
        bd_addr_to_string(paired_devices[i].bd_addr, addr_str);
        ESP_LOGI(TAG, "Loaded device %d: %s (%s)", i, paired_devices[i].device_name, addr_str);
    }

    ESP_LOGI(TAG, "Loaded %d paired devices from NVS", paired_device_count);
    return ESP_OK;
}

// Сохранение устройств в NVS
static esp_err_t save_devices_to_nvs(void) {
    esp_err_t err;
    
    // Сохраняем количество устройств
    err = nvs_set_blob(nvs_handle_storage, NVS_KEY_COUNT, &paired_device_count, sizeof(paired_device_count));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save device count to NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Сохраняем каждое устройство
    for (int i = 0; i < paired_device_count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_DEVICE_PREFIX, i);
        
        err = nvs_set_blob(nvs_handle_storage, key, &paired_devices[i], sizeof(paired_device_t));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save device %d to NVS: %s", i, esp_err_to_name(err));
            return err;
        }
    }

    // Коммитим изменения
    err = nvs_commit(nvs_handle_storage);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Saved %d paired devices to NVS", paired_device_count);
    return ESP_OK;
}

esp_err_t paired_devices_init(void) {
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle_storage);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }

    // Загружаем устройства из NVS
    err = load_devices_from_nvs();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load devices from NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Paired devices module initialized with %d devices", paired_device_count);
    return ESP_OK;
}

esp_err_t paired_devices_add(const esp_bd_addr_t bd_addr, const char *device_name, uint32_t cod, bool is_hf_device) {
    if (paired_device_count >= MAX_PAIRED_DEVICES) {
        ESP_LOGW(TAG, "Maximum number of paired devices reached (%d)", MAX_PAIRED_DEVICES);
        return ESP_ERR_NO_MEM;
    }

    // Проверяем, не существует ли уже такое устройство
    for (int i = 0; i < paired_device_count; i++) {
        if (bd_addr_equal(paired_devices[i].bd_addr, bd_addr)) {
            // Обновляем существующее устройство
            strncpy(paired_devices[i].device_name, device_name ? device_name : "", DEVICE_NAME_MAX_LEN - 1);
            paired_devices[i].device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';
            paired_devices[i].cod = cod;
            paired_devices[i].is_hf_device = is_hf_device;
            paired_devices[i].last_connected_time = time(NULL);
            paired_devices[i].connection_count++;
            
            char addr_str[18];
            bd_addr_to_string(bd_addr, addr_str);
            ESP_LOGI(TAG, "Updated existing device: %s (%s)", paired_devices[i].device_name, addr_str);
            
            return save_devices_to_nvs();
        }
    }

    // Добавляем новое устройство
    paired_device_t *new_device = &paired_devices[paired_device_count];
    memcpy(new_device->bd_addr, bd_addr, ESP_BD_ADDR_LEN);
    strncpy(new_device->device_name, device_name ? device_name : "", DEVICE_NAME_MAX_LEN - 1);
    new_device->device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';
    new_device->cod = cod;
    new_device->is_hf_device = is_hf_device;
    new_device->last_connected_time = time(NULL);
    new_device->connection_count = 1;

    paired_device_count++;

    char addr_str[18];
    bd_addr_to_string(bd_addr, addr_str);
    ESP_LOGI(TAG, "Added new device: %s (%s), HF: %s", new_device->device_name, addr_str, is_hf_device ? "Yes" : "No");

    return save_devices_to_nvs();
}

esp_err_t paired_devices_remove(const esp_bd_addr_t bd_addr) {
    for (int i = 0; i < paired_device_count; i++) {
        if (bd_addr_equal(paired_devices[i].bd_addr, bd_addr)) {
            char addr_str[18];
            bd_addr_to_string(bd_addr, addr_str);
            ESP_LOGI(TAG, "Removing device: %s (%s)", paired_devices[i].device_name, addr_str);
            
            // Сдвигаем все элементы после удаляемого
            for (int j = i; j < paired_device_count - 1; j++) {
                paired_devices[j] = paired_devices[j + 1];
            }
            paired_device_count--;
            
            return save_devices_to_nvs();
        }
    }

    char addr_str[18];
    bd_addr_to_string(bd_addr, addr_str);
    ESP_LOGW(TAG, "Device not found for removal: %s", addr_str);
    return ESP_ERR_NOT_FOUND;
}

paired_device_t* paired_devices_find(const esp_bd_addr_t bd_addr) {
    for (int i = 0; i < paired_device_count; i++) {
        if (bd_addr_equal(paired_devices[i].bd_addr, bd_addr)) {
            return &paired_devices[i];
        }
    }
    return NULL;
}

int paired_devices_count(void) {
    return paired_device_count;
}

int paired_devices_get_all(paired_device_t *devices, int max_count) {
    int count = paired_device_count < max_count ? paired_device_count : max_count;
    for (int i = 0; i < count; i++) {
        devices[i] = paired_devices[i];
    }
    return count;
}

esp_err_t paired_devices_update_connection_time(const esp_bd_addr_t bd_addr) {
    paired_device_t *device = paired_devices_find(bd_addr);
    if (device) {
        device->last_connected_time = time(NULL);
        device->connection_count++;
        return save_devices_to_nvs();
    }
    return ESP_ERR_NOT_FOUND;
}

paired_device_t* paired_devices_get_reconnect_candidate(void) {
    paired_device_t *candidate = NULL;
    uint32_t latest_time = 0;

    // Находим HF устройство с самым поздним временем подключения
    for (int i = 0; i < paired_device_count; i++) {
        if (paired_devices[i].is_hf_device && paired_devices[i].last_connected_time > latest_time) {
            latest_time = paired_devices[i].last_connected_time;
            candidate = &paired_devices[i];
        }
    }

    return candidate;
}

esp_err_t paired_devices_clear_all(void) {
    ESP_LOGI(TAG, "Clearing all paired devices");
    paired_device_count = 0;
    
    // Очищаем NVS
    esp_err_t err = nvs_erase_all(nvs_handle_storage);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear NVS: %s", esp_err_to_name(err));
        return err;
    }

    return nvs_commit(nvs_handle_storage);
}

void paired_devices_print_list(void) {
    ESP_LOGI(TAG, "=== Paired Devices List (%d devices) ===", paired_device_count);
    
    if (paired_device_count == 0) {
        ESP_LOGI(TAG, "No paired devices");
        return;
    }

    for (int i = 0; i < paired_device_count; i++) {
        char addr_str[18];
        bd_addr_to_string(paired_devices[i].bd_addr, addr_str);
        
        ESP_LOGI(TAG, "%d. %s (%s)", i + 1, paired_devices[i].device_name, addr_str);
        ESP_LOGI(TAG, "   HF: %s, COD: 0x%06lx, Connections: %lu",
                 paired_devices[i].is_hf_device ? "Yes" : "No",
                 (unsigned long)paired_devices[i].cod,
                 (unsigned long)paired_devices[i].connection_count);
    }
    
    ESP_LOGI(TAG, "=== End of Paired Devices List ===");
}

esp_err_t paired_devices_get_last_connected(esp_bd_addr_t bd_addr) {
    if (bd_addr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    paired_device_t* device = paired_devices_get_reconnect_candidate();
    if (device == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    
    memcpy(bd_addr, device->bd_addr, sizeof(esp_bd_addr_t));
    return ESP_OK;
}

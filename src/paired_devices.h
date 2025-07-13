#ifndef PAIRED_DEVICES_H
#define PAIRED_DEVICES_H

#include <stdbool.h>
#include "esp_bt_defs.h"
#include "esp_gap_bt_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PAIRED_DEVICES 10
#define DEVICE_NAME_MAX_LEN 64

typedef struct {
    esp_bd_addr_t bd_addr;
    char device_name[DEVICE_NAME_MAX_LEN];
    uint32_t cod;  // Class of Device
    bool is_hf_device;
    uint32_t last_connected_time;
    uint32_t connection_count;
} paired_device_t;

/**
 * @brief Инициализация модуля сопряженных устройств
 * @return ESP_OK при успехе
 */
esp_err_t paired_devices_init(void);

/**
 * @brief Добавление устройства в список сопряженных
 * @param bd_addr MAC адрес устройства
 * @param device_name Имя устройства
 * @param cod Class of Device
 * @param is_hf_device Является ли устройство HF
 * @return ESP_OK при успехе
 */
esp_err_t paired_devices_add(const esp_bd_addr_t bd_addr, const char *device_name, uint32_t cod, bool is_hf_device);

/**
 * @brief Удаление устройства из списка сопряженных
 * @param bd_addr MAC адрес устройства
 * @return ESP_OK при успехе
 */
esp_err_t paired_devices_remove(const esp_bd_addr_t bd_addr);

/**
 * @brief Поиск устройства в списке сопряженных
 * @param bd_addr MAC адрес устройства
 * @return Указатель на устройство или NULL если не найдено
 */
paired_device_t* paired_devices_find(const esp_bd_addr_t bd_addr);

/**
 * @brief Получение количества сопряженных устройств
 * @return Количество устройств
 */
int paired_devices_count(void);

/**
 * @brief Получение списка всех сопряженных устройств
 * @param devices Массив для записи устройств
 * @param max_count Максимальное количество устройств
 * @return Количество записанных устройств
 */
int paired_devices_get_all(paired_device_t *devices, int max_count);

/**
 * @brief Обновление времени последнего подключения
 * @param bd_addr MAC адрес устройства
 * @return ESP_OK при успехе
 */
esp_err_t paired_devices_update_connection_time(const esp_bd_addr_t bd_addr);

/**
 * @brief Получение устройства для автоматического переподключения
 * @return Указатель на устройство или NULL если нет кандидатов
 */
paired_device_t* paired_devices_get_reconnect_candidate(void);

/**
 * @brief Очистка всех сопряженных устройств
 * @return ESP_OK при успехе
 */
esp_err_t paired_devices_clear_all(void);

/**
 * @brief Вывод списка сопряженных устройств в лог
 */
void paired_devices_print_list(void);

/**
 * @brief Получение последнего подключенного устройства
 * @param bd_addr Буфер для записи MAC адреса устройства
 * @return ESP_OK при успехе
 */
esp_err_t paired_devices_get_last_connected(esp_bd_addr_t bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* PAIRED_DEVICES_H */

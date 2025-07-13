#ifndef AUTO_RECONNECT_H
#define AUTO_RECONNECT_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_bt_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUTO_RECONNECT_INTERVAL_MS 10000  // 10 секунд
#define AUTO_RECONNECT_MAX_ATTEMPTS 5     // Максимум попыток

typedef enum {
    AUTO_RECONNECT_STATE_IDLE,
    AUTO_RECONNECT_STATE_SEARCHING,
    AUTO_RECONNECT_STATE_CONNECTING,
    AUTO_RECONNECT_STATE_CONNECTED,
    AUTO_RECONNECT_STATE_FAILED
} auto_reconnect_state_t;

/**
 * @brief Инициализация модуля автоматического переподключения
 * @return ESP_OK при успехе
 */
esp_err_t auto_reconnect_init(void);

/**
 * @brief Запуск автоматического переподключения
 * @return ESP_OK при успехе
 */
void auto_reconnect_start(void);

/**
 * @brief Остановка автоматического переподключения
 * @return ESP_OK при успехе
 */
void auto_reconnect_stop(void);

/**
 * @brief Уведомление о состоянии подключения
 * @param connected true если подключено, false если отключено
 */
void auto_reconnect_notify_connection_state(bool connected);

/**
 * @brief Уведомление о завершении поиска устройств
 */
void auto_reconnect_notify_discovery_complete(void);

/**
 * @brief Уведомление о неудачном подключении
 */
void auto_reconnect_notify_connection_failed(void);

/**
 * @brief Уведомление о найденном устройстве
 * @param bd_addr Адрес устройства
 * @param name Имя устройства
 * @param cod Код класса устройства
 */
void auto_reconnect_notify_device_found(const esp_bd_addr_t bd_addr, const char* name, uint32_t cod);

/**
 * @brief Получение текущего состояния автоматического переподключения
 * @return Текущее состояние
 */
auto_reconnect_state_t auto_reconnect_get_state(void);

/**
 * @brief Получение количества попыток переподключения
 * @return Количество попыток
 */
int auto_reconnect_get_attempts(void);

/**
 * @brief Сброс счетчика попыток переподключения
 */
void auto_reconnect_reset_attempts(void);

/**
 * @brief Проверка, включено ли автоматическое переподключение
 * @return true если включено
 */
bool auto_reconnect_is_enabled(void);

/**
 * @brief Проверка, активно ли автоматическое переподключение
 * @return true если активно
 */
bool auto_reconnect_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* AUTO_RECONNECT_H */

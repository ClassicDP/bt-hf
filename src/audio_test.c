#include "audio_test.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_hf_ag_api.h"
#include <string.h>
#include <math.h>

static const char *TAG = "AUDIO_TEST";

// Глобальная статистика
static audio_stats_t audio_stats = {0};
static bool monitoring_active = false;
static TimerHandle_t stats_timer = NULL;

// Задача для мониторинга аудио
static void audio_monitoring_task(void *param);
static TaskHandle_t monitoring_task_handle = NULL;

// Таймер для периодического вывода статистики
static void stats_timer_callback(TimerHandle_t xTimer)
{
    if (monitoring_active) {
        audio_test_print_stats();
    }
}

void audio_test_init(void)
{
    ESP_LOGI(TAG, "🔧 Initializing audio test module");

    // Сброс статистики
    memset(&audio_stats, 0, sizeof(audio_stats));

    // Создаем таймер для периодического вывода статистики (каждые 10 секунд)
    stats_timer = xTimerCreate("audio_stats",
                              pdMS_TO_TICKS(10000),
                              pdTRUE,
                              NULL,
                              stats_timer_callback);

    if (stats_timer == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create stats timer");
        return;
    }

    ESP_LOGI(TAG, "✅ Audio test module initialized");
}

void audio_test_start_monitoring(void)
{
    if (monitoring_active) {
        ESP_LOGW(TAG, "⚠️ Audio monitoring already active");
        return;
    }

    ESP_LOGI(TAG, "🎵 Starting audio monitoring");
    monitoring_active = true;

    // Запускаем задачу мониторинга
    xTaskCreate(audio_monitoring_task, "audio_monitor", 4096, NULL, 5, &monitoring_task_handle);

    // Запускаем таймер статистики
    if (stats_timer != NULL) {
        xTimerStart(stats_timer, 0);
    }
}

void audio_test_stop_monitoring(void)
{
    if (!monitoring_active) {
        ESP_LOGW(TAG, "⚠️ Audio monitoring not active");
        return;
    }

    ESP_LOGI(TAG, "🛑 Stopping audio monitoring");
    monitoring_active = false;

    // Останавливаем таймер
    if (stats_timer != NULL) {
        xTimerStop(stats_timer, 0);
    }

    // Завершаем задачу мониторинга
    if (monitoring_task_handle != NULL) {
        vTaskDelete(monitoring_task_handle);
        monitoring_task_handle = NULL;
    }
}

static void audio_monitoring_task(void *param)
{
    ESP_LOGI(TAG, "🎧 Audio monitoring task started");

    while (monitoring_active) {
        // Проверяем состояние аудио соединения
        if (audio_stats.is_sco_active) {
            ESP_LOGD(TAG, "🔊 SCO connection active - monitoring audio quality");

            // Здесь можно добавить проверки качества звука
            // Например, проверка уровня сигнала, задержки и т.д.
        }

        // Проверяем каждые 2 секунды
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "🎧 Audio monitoring task stopped");
    vTaskDelete(NULL);
}

void audio_test_print_stats(void)
{
    ESP_LOGI(TAG, "📊 === AUDIO STATISTICS ===");
    ESP_LOGI(TAG, "SCO Connections: %lu", audio_stats.sco_connections);
    ESP_LOGI(TAG, "SCO Disconnections: %lu", audio_stats.sco_disconnections);
    ESP_LOGI(TAG, "Audio Packets Sent: %lu", audio_stats.audio_packets_sent);
    ESP_LOGI(TAG, "Audio Packets Received: %lu", audio_stats.audio_packets_received);
    ESP_LOGI(TAG, "Codec Switches: %lu", audio_stats.codec_switches);
    ESP_LOGI(TAG, "mSBC Active: %s", audio_stats.is_msbc_active ? "Yes" : "No");
    ESP_LOGI(TAG, "SCO Active: %s", audio_stats.is_sco_active ? "Yes" : "No");
    ESP_LOGI(TAG, "========================");
}

void audio_test_reset_stats(void)
{
    ESP_LOGI(TAG, "🔄 Resetting audio statistics");
    memset(&audio_stats, 0, sizeof(audio_stats));
}

esp_err_t audio_test_connect_sco(esp_bd_addr_t remote_addr)
{
    ESP_LOGI(TAG, "🔊 Testing SCO connection");

    esp_err_t ret = esp_hf_ag_audio_connect(remote_addr);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ SCO connection request sent successfully");
    } else {
        ESP_LOGE(TAG, "❌ Failed to send SCO connection request: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t audio_test_disconnect_sco(esp_bd_addr_t remote_addr)
{
    ESP_LOGI(TAG, "🔇 Testing SCO disconnection");

    esp_err_t ret = esp_hf_ag_audio_disconnect(remote_addr);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ SCO disconnection request sent successfully");
    } else {
        ESP_LOGE(TAG, "❌ Failed to send SCO disconnection request: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t audio_test_send_tone(esp_bd_addr_t remote_addr, uint16_t frequency, uint32_t duration_ms)
{
    ESP_LOGI(TAG, "🎵 Testing tone generation: %d Hz for %lu ms", frequency, duration_ms);

    // Примечание: ESP-IDF не предоставляет прямую функцию для генерации тонов
    // через HFP AG API. Это нужно реализовывать через низкоуровневые PCM данные

    ESP_LOGW(TAG, "⚠️ Tone generation requires custom PCM data implementation");
    ESP_LOGI(TAG, "💡 Consider implementing PCM tone generation for frequency %d Hz", frequency);

    return ESP_OK;
}

esp_err_t audio_test_volume_control(esp_bd_addr_t remote_addr, int volume)
{
    ESP_LOGI(TAG, "🔊 Testing volume control: level %d", volume);

    // Проверяем диапазон громкости (обычно 0-15 для HFP)
    if (volume < 0 || volume > 15) {
        ESP_LOGE(TAG, "❌ Invalid volume level: %d (valid range: 0-15)", volume);
        return ESP_ERR_INVALID_ARG;
    }

    // В HFP AG роли, громкость обычно устанавливается устройством HF
    // Но мы можем отправить индикацию об изменении громкости
    ESP_LOGI(TAG, "📢 Volume level %d would be applied to SCO connection", volume);

    return ESP_OK;
}

void audio_test_handle_sco_connected(void)
{
    ESP_LOGI(TAG, "✅ SCO Connected - Audio channel established");
    audio_stats.sco_connections++;
    audio_stats.is_sco_active = true;

    // Выводим информацию о соединении
    ESP_LOGI(TAG, "🎉 Audio test: SCO connection #%lu established", audio_stats.sco_connections);
}

void audio_test_handle_sco_disconnected(void)
{
    ESP_LOGI(TAG, "❌ SCO Disconnected - Audio channel closed");
    audio_stats.sco_disconnections++;
    audio_stats.is_sco_active = false;

    // Выводим информацию о разрыве соединения
    ESP_LOGI(TAG, "📊 Audio test: SCO disconnection #%lu", audio_stats.sco_disconnections);
}

void audio_test_handle_codec_change(bool is_msbc)
{
    ESP_LOGI(TAG, "🎵 Codec changed to: %s", is_msbc ? "mSBC (HD)" : "CVSD");
    audio_stats.codec_switches++;
    audio_stats.is_msbc_active = is_msbc;

    if (is_msbc) {
        ESP_LOGI(TAG, "🎧 High-quality mSBC codec is now active");
    } else {
        ESP_LOGI(TAG, "🎧 Standard CVSD codec is now active");
    }
}

audio_stats_t* audio_test_get_stats(void)
{
    return &audio_stats;
}

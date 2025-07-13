#include "audio_handler.h"
#include "esp_log.h"
#include "esp_hf_ag_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>

static const char *TAG = "AUDIO_HANDLER";

static uint16_t s_sync_conn_handle = 0;
static bool s_audio_connected = false;
static bool s_msbc_mode = false;

// Callback для входящих аудио данных (с микрофона устройства)
static void audio_data_callback(const uint8_t *data, uint32_t len)
{
    ESP_LOGI(TAG, "📡 Received audio data: %" PRIu32 " bytes", len);
    
    // Здесь можно обработать полученные аудио данные
    // Например, сохранить в буфер, записать в файл, отправить по сети и т.д.
    
    // Пример простого логирования первых нескольких байт
    if (len >= 4) {
        ESP_LOGI(TAG, "First 4 bytes: %02X %02X %02X %02X", 
                 data[0], data[1], data[2], data[3]);
    }
}

// Callback для исходящих аудио данных (в динамик устройства)
static uint32_t audio_outgoing_callback(uint8_t *buf, uint32_t len)
{
    if (!s_audio_connected) {
        // Заполняем буфер тишиной даже если не подключено
        memset(buf, 0, len);
        return len;
    }

    ESP_LOGI(TAG, "📤 Sending audio data: %" PRIu32 " bytes", len);
    
    // Пример: генерируем простой тестовый сигнал (синусоиду)
    static uint32_t sample_count = 0;
    int16_t *samples = (int16_t *)buf;
    uint32_t sample_len = len / 2; // 16-bit samples
    
    for (uint32_t i = 0; i < sample_len; i++) {
        // Генерируем 440 Hz тон на частоте 8 kHz (для CVSD) или 16 kHz (для mSBC)
        double sample_rate = s_msbc_mode ? 16000.0 : 8000.0;
        double phase = 2.0 * M_PI * 440.0 * (sample_count + i) / sample_rate;
        samples[i] = (int16_t)(8000.0 * sin(phase)); // Уменьшаем амплитуду для комфортного звука
    }
    
    sample_count += sample_len;
    
    return len;
}

void audio_handler_init(void)
{
    ESP_LOGI(TAG, "Initializing audio handler for HCI data path...");
    
    // Регистрируем callback для HCI данных
    esp_err_t ret = esp_hf_ag_register_data_callback(audio_data_callback, audio_outgoing_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register HCI data callback: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "✅ Audio handler initialized successfully");
}

void audio_handler_set_connection_state(bool connected, uint16_t sync_conn_hdl, bool msbc_mode)
{
    s_audio_connected = connected;
    s_sync_conn_handle = sync_conn_hdl;
    s_msbc_mode = msbc_mode;
    
    ESP_LOGI(TAG, "Audio connection state: %s, sync_conn_hdl: %d, codec: %s", 
             connected ? "CONNECTED" : "DISCONNECTED", 
             sync_conn_hdl,
             msbc_mode ? "mSBC" : "CVSD");
    
    if (connected) {
        ESP_LOGI(TAG, "🎙️ Audio recording is now available!");
        ESP_LOGI(TAG, "📊 Audio will be processed through HCI callbacks");
        ESP_LOGI(TAG, "🔊 Sample rate: %s", msbc_mode ? "16 kHz" : "8 kHz");
        ESP_LOGI(TAG, "🎵 Ready to process audio data!");
    } else {
        ESP_LOGI(TAG, "🔇 Audio processing stopped");
    }
}

void audio_handler_send_test_audio(void)
{
    if (!s_audio_connected) {
        ESP_LOGW(TAG, "Audio not connected, cannot send test audio");
        return;
    }
    
    ESP_LOGI(TAG, "🔊 Test audio will be generated in outgoing callback");
    ESP_LOGI(TAG, "📈 Codec: %s, Handle: %d", s_msbc_mode ? "mSBC" : "CVSD", s_sync_conn_handle);
}

bool audio_handler_is_connected(void)
{
    return s_audio_connected;
}

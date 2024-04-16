/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <string.h>
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "tts_api.h"
#include "esp_http_client.h"
#include "app_audio.h"
#include "app_ui_ctrl.h"
#include "audio_player.h"
#include "esp_crt_bundle.h"
#include "inttypes.h"

#define VOICE_ID CONFIG_VOICE_ID
#define VOLUME CONFIG_VOLUME_LEVEL
#define XI_API_KEY CONFIG_XI_API_KEY

static const char *TAG = "TTS-Api";

/* Define a function to handle HTTP events during an HTTP request */

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        file_total_len = 0;
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=(%" PRIu32 " + %d) [%d]", file_total_len, evt->data_len, MAX_FILE_SIZE);
        if ((file_total_len + evt->data_len) < MAX_FILE_SIZE)
        {
            memcpy(audio_rx_buffer + file_total_len, (char *)evt->data, evt->data_len);
            file_total_len += evt->data_len;
        }
        if (!esp_http_client_is_chunked_response(evt->client)) 
        {
            printf("%.*s", evt->data_len, (char*)evt->data);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH:%" PRIu32 ", %" PRIu32 " K", file_total_len, file_total_len / 1024);
        audio_player_play(audio_rx_buffer, file_total_len);
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

/* Create Text to Speech request */

esp_err_t text_to_speech_request(const char *message)
{
    int url_size = snprintf(NULL, 0, "https://api.elevenlabs.io/v1/text-to-speech/%s/stream", VOICE_ID);
    // Allocate memory for the URL buffer
    char *url = heap_caps_malloc((url_size + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (url == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for URL");
        return ESP_ERR_NO_MEM;
    }
    snprintf(url, url_size + 1, "https://api.elevenlabs.io/v1/text-to-speech/%s/stream", VOICE_ID);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .buffer_size = 128000,
        .buffer_size_tx = 4000,
        .timeout_ms = 40000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    uint32_t starttime = esp_log_timestamp();
    ESP_LOGE(TAG, "[Start] create_TTS_request, timestamp:%" PRIu32, starttime);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "xi-api-key", XI_API_KEY);
    char *json_data =
        "{\"text\": \"%s\", \"model_id\": \"eleven_multilingual_v2\", "
        "\"voice_settings\": {\"stability\": 0.5, \"similarity_boost\": 0.8, "
        "\"style\": 0.0, \"use_speaker_boost\": true}}";
    int json_data_size = snprintf(NULL, 0, json_data, message);
    // Allocate memory for the json_payload buffer
    char *json_payload = heap_caps_malloc((json_data_size + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (json_payload == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for json_payload");
        return ESP_ERR_NO_MEM;
    }
    snprintf(json_payload, json_data_size + 1, json_data, message);  
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    ESP_LOGE(TAG, "[End] create_TTS_request, + offset:%" PRIu32, esp_log_timestamp() - starttime);

    heap_caps_free(url);
    esp_http_client_cleanup(client);
    return err;
}

/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <cJSON.h>
#include <stdio.h>

#include <string>

#include "driver/gpio.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_websocket_client.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "tcp_client.hpp"


#define NO_DATA_TIMEOUT_SEC 5
#define BLINK_GPIO GPIO_NUM_2


static const char* TAG = "main";
static const std::string RECEIVED_MSG = "received";
static const uint16_t HOST_PORT = static_cast<uint16_t>(std::stoi(CONFIG_TCP_HOST_IP_PORT));
static TimerHandle_t shutdownSignalTimer;
static SemaphoreHandle_t shutdownSema;


static void logErrorIfNonzero(const char* message, int errorCode)
{
    if (errorCode != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, errorCode);
    }
}


static void shutdownSignaler(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdownSema);
}


static void websocketEventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventId, void* eventData)
{
    auto client = reinterpret_cast<esp_websocket_client_handle_t>(handlerArgs);
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) eventData;
    switch (eventId)
    {
        case WEBSOCKET_EVENT_BEGIN:
        {
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
            break;
        }
        case WEBSOCKET_EVENT_CONNECTED:
        {
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            break;
        }
        case WEBSOCKET_EVENT_DISCONNECTED:
        {
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            logErrorIfNonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
            if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
            {
                logErrorIfNonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
                logErrorIfNonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
                logErrorIfNonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
            }
            break;
        }
        case WEBSOCKET_EVENT_DATA:
        {
            // ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
            // ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
            if (data->op_code == 0x2)
            {  // Opcode 0x2 indicates binary data
                ESP_LOG_BUFFER_HEX("Received binary data", data->data_ptr, data->data_len);
            }
            else if (data->op_code == 0x08 && data->data_len == 2)
            {
                ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
            }
            else
            {
                // ESP_LOGW(TAG, "Received=%.*s\n\n", data->data_len, (char *)data->data_ptr);
                esp_websocket_client_send_text(client, RECEIVED_MSG.data(), RECEIVED_MSG.size(), pdMS_TO_TICKS(50));
                // ESP_LOGI(TAG, "Sent back ping");
            }

            // If received data contains json structure it succeed to parse
            cJSON* root = cJSON_Parse(data->data_ptr);
            if (root)
            {
                for (int i = 0; i < cJSON_GetArraySize(root); i++)
                {
                    cJSON* elem = cJSON_GetArrayItem(root, i);
                    cJSON* id = cJSON_GetObjectItem(elem, "id");
                    cJSON* name = cJSON_GetObjectItem(elem, "name");
                    ESP_LOGW(TAG, "Json={'id': '%s', 'name': '%s'}", id->valuestring, name->valuestring);
                }
                cJSON_Delete(root);
            }

            ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

            xTimerReset(shutdownSignalTimer, portMAX_DELAY);
            break;
        }
        case WEBSOCKET_EVENT_ERROR:
        {
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
            logErrorIfNonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
            if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
            {
                logErrorIfNonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
                logErrorIfNonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
                logErrorIfNonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
            }
            break;
        }
        case WEBSOCKET_EVENT_FINISH:
        {
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
            break;
        }
    }
}


static void websocketAppStart()
{
    esp_websocket_client_config_t websocketCfg = {};
    shutdownSignalTimer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
        pdFALSE, NULL, shutdownSignaler);
    shutdownSema = xSemaphoreCreateBinary();
    websocketCfg.uri = CONFIG_WEBSOCKET_URI;
    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocketCfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocketEventHandler, (void*) client);
    esp_websocket_client_start(client);

    xSemaphoreTake(shutdownSema, portMAX_DELAY);
    esp_websocket_client_close(client, portMAX_DELAY);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(client);
    return;
}


__attribute__((unused)) static void tcpAppStart()
{
    // Configure the GPIO

    TcpClient client{CONFIG_TCP_HOST_IP_ADDR, HOST_PORT};
    client.connectToServer();
    std::string expectedMsg = "button clicked\n";
    std::string buffer(expectedMsg.size(), '\0');
    while (true)
    {
        auto err = client.receiveData(buffer);
        if (err)
        {
            // error
            break;
        }
        ESP_LOGI(TAG, "Received %s", buffer.c_str());
        err = client.sendData(RECEIVED_MSG);
        if (err)
        {
            break;
        }
    }
    xSemaphoreTake(shutdownSema, portMAX_DELAY);
    ESP_LOGE(TAG, "Tcp client stopped");
}


extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("websocket_client", ESP_LOG_DEBUG);
    esp_log_level_set("transport_ws", ESP_LOG_DEBUG);
    esp_log_level_set("trans_tcp", ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */

    ESP_ERROR_CHECK(example_connect());

    // Setting wifi power saving off is EXTREMELY important for low latency.
    // Basic LAN ping goes from 100-500ms down to <10ms with this setting.
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    websocketAppStart();
    // tcpAppStart();
}

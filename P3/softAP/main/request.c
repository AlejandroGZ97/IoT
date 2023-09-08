#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_tls.h"

#define SSID ""
#define PASS ""

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
        break;
    }
}

void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation                  s1.1
    esp_event_loop_create_default();     // event loop                         s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //                                     s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS}};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
}

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

static void rest_get()
{
    esp_http_client_config_t config_get = {
        .url = "https://maker.ifttt.com/trigger/sensor_temperatura/json/with/key/",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler
    };

    // Configura opciones TLS
    esp_tls_cfg_t tls_cfg = {
        .cacert_buf = NULL,  // Aquí puedes proporcionar un certificado CA para verificar el servidor}
        .psk_hint_key = NULL,
        .alpn_protos = NULL,
        .common_name = "maker.ifttt.com"  // El nombre de host del servidor
    };

    //config_get.tls_cfg = &tls_cfg;  // Asigna la configuración TLS al cliente 
    //esp_tls_t *tls_cfg = esp_tls_init();

    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    #ifdef CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS
    esp_transport_handle_t ssl;
    _success = (
                   (ssl = esp_transport_ssl_init()) &&
                   (esp_transport_set_default_port(ssl, DEFAULT_HTTPS_PORT) == ESP_OK) &&
                   (esp_transport_list_add(client->transport_list, ssl, "https") == ESP_OK)
               );
    if (!_success) {
        esp_http_client_cleanup(client);
        return NULL;
    }
    if (config->cert_pem) {
        esp_transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
    }
#endif
    esp_http_client_perform(client);
    //esp_http_client_cleanup(client);
}

void app_main(void)
{
    nvs_flash_init();
    wifi_connection();

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");

    rest_get();
    vTaskDelay(20000 / portTICK_PERIOD_MS);
}
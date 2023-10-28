#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "sensor_pt2.c"
#include "statistics.c"

#define SSID "P_AGZ"
#define PASS "clavePrac"
#define BROKER_URL  "mqtts://test.mosquitto.org:8883/"
#define MY_TOPIC    "Ale/sensor_temp"
#define REQ_DATA    "ReqData"
#define SUB_TOPIC   "sensors/drone05/ReqStatistics"
#define PUB_TOPIC   "sensors/drone05/Statistics"
#define TEMP_MSG_SIZE   25

static const char *TAG = "MQTTS";
esp_mqtt_client_handle_t current_client;
int msg_id;

//net start/stop mosquitto
//mosquitto -c "C:\Program Files\mosquitto\mosquitto.conf"
//subscribe> mosquitto_sub -h test.mosquitto.org -t "Ale/sensor_temp" -p 8884 --cafile C:\certificados2\mosquitto.org.crt --key C:\certificados2\client.key --cert C:\certificados2\client.crt -d
//subscribe stats> mosquitto_sub -h test.mosquitto.org -t "sensors/drone05/Statistics" -p 8884 --cafile C:\certificados2\mosquitto.org.crt --key C:\certificados2\client.key --cert C:\certificados2\client.crt -d
//publish> mosquitto_pub --cafile C:\certificados2\mosquitto.org.crt --key C:\certificados2\client.key --cert C:\certificados2\client.crt -h test.mosquitto.org -m "ReqData" -t "sensors/drone05/ReqStatistics"

extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

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
    esp_netif_init();                    // TCP/IP initiation
    esp_event_loop_create_default();     // event loop
    esp_netif_create_default_wifi_sta(); // WiFi station 
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); 
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

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, SUB_TOPIC, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        sprintf(reqData,"%.*s",event->data_len, event->data);
        if (statsFlag && !strcmp(reqData,REQ_DATA))
        {
            sprintf(allStats,"Media: %f Mediana: %f Varianza: %f",mean(),median(),variance(mean()));
            msg_id = esp_mqtt_client_publish(client, PUB_TOPIC, allStats,0,1,0);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URL,
        .cert_pem = (const char *)mqtt_eclipseprojects_io_pem_start
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    current_client = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    float temperature;
    char temp[TEMP_MSG_SIZE];

    nvs_flash_init();
    wifi_connection();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");

    mqtt_app_start();

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG2, "Inicializado I2C master");
    aht10_calibrate();
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    while (1)
    {
        aht10_read_data(data);
        temperature = aht10_calculate_temperature(data);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG2, "Temperatura: %.2f", temperature);
        sprintf(temp,"Temperatura : %f",temperature);

        msg_id = esp_mqtt_client_publish(current_client, MY_TOPIC, temp,0,1,0);

        tempValues[valuesPos] = temperature;
        valuesPos++;
        if (valuesPos == SIZE)
        {
            valuesPos = 0;
            statsFlag = true;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

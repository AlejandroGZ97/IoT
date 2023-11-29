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

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "driver/gpio.h"

#include "aht10.c"

//#define SSID "P_AGZ"
//#define PASS "clavePrac"
#define SSID "3PesosDeInternet"
#define PASS "Son3pesos."
#define BROKER_URL  "mqtt://test.mosquitto.org:1883"
#define MY_TOPIC    "v1/devices/alejandro/temp"
#define RECV_TOPIC  "v1/devices/seralmar/attributes"
#define TEMP_MSG_SIZE   25
#define RECV_MSG        40
#define STR_SIZE        10

#define LED_ACC     GPIO_NUM_13
#define LED_BREAK   GPIO_NUM_12
#define LED_LEFT    GPIO_NUM_15
#define LED_RIGHT   GPIO_NUM_4

static const char *TAG = "MQTT_TCP";
esp_mqtt_client_handle_t current_client;
int msg_id;
char recv_msg[RECV_MSG], recv_check[STR_SIZE];

//net start/stop mosquitto
//publish> mosquitto_pub -h test.mosquitto.org -p 1883 -t v1/devices/seralmar/attributes -m "{"Action":"Accelerate","Value":"100"}" -d
//mosquitto_pub -h test.mosquitto.org -p 1883 -t v1/devices/seralmar/attributes -m "{"Action":"Break","Value":"100"}" -d
//mosquitto_pub -h test.mosquitto.org -p 1883 -t v1/devices/seralmar/attributes -m "{"Action":"TurnLeft","Value":"100"}" -d
//mosquitto_pub -h test.mosquitto.org -p 1883 -t v1/devices/seralmar/attributes -m "{"Action":"TurnRight","Value":"100"}" -d

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
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, RECV_TOPIC, 0);
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
        //sprintf("TOPIC=%.*s\r", event->topic_len, event->topic);
        printf("%.*s\r\n", event->data_len, event->data);
        sprintf(recv_msg,"%.*s\r", event->data_len, event->data);
        for (int i = 11, j = 0; recv_msg[i] != '"'; i++)
        {
            recv_check[j] = recv_msg[i];
            ESP_LOGI(TAG, "%c",recv_check[j]);
            j++;
            if (recv_msg[i] == ',')
            {
                recv_check[j+1] = 0;
            }
            
        }
        
        if (!strcmp(recv_check,"Accelerate"))
        {
            gpio_set_level(LED_ACC,1);
            gpio_set_level(LED_BREAK,0);
            gpio_set_level(LED_LEFT,0);
            gpio_set_level(LED_RIGHT,0);
        }
        if (!strcmp(recv_check,"Break"))
        {
            gpio_set_level(LED_ACC,0);
            gpio_set_level(LED_BREAK,1);
            gpio_set_level(LED_LEFT,0);
            gpio_set_level(LED_RIGHT,0);
        }
        if (!strcmp(recv_check,"TurnLeft"))
        {
            gpio_set_level(LED_ACC,0);
            gpio_set_level(LED_BREAK,0);
            gpio_set_level(LED_LEFT,1);
            gpio_set_level(LED_RIGHT,0);
        }
        if (!strcmp(recv_check,"TurnRight"))
        {
            gpio_set_level(LED_ACC,0);
            gpio_set_level(LED_BREAK,0);
            gpio_set_level(LED_LEFT,0);
            gpio_set_level(LED_RIGHT,1);
        }
        
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URL
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    current_client = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

static void initIO(void)
{
    // code to initialize all IO pins for the assigment
    gpio_reset_pin(LED_ACC);
    gpio_reset_pin(LED_BREAK);
    gpio_reset_pin(LED_LEFT);
    gpio_reset_pin(LED_RIGHT);
    /* Set LED GPIO as a push/pull output */
    gpio_set_direction(LED_ACC, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BREAK, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_LEFT, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_RIGHT, GPIO_MODE_OUTPUT);
    /* Leds in 0 */
    gpio_set_level(LED_ACC,1);
    gpio_set_level(LED_BREAK,1);
    gpio_set_level(LED_LEFT,1);
    gpio_set_level(LED_RIGHT,1);
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
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG2, "Inicializado I2C master");
    aht10_calibrate();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    initIO();

    while (1)
    {
        aht10_read_data(data);
        temperature = aht10_calculate_temperature(data);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG2, "Temperatura: %.3f", temperature);
        sprintf(temp,"{temperature:%.3f}",temperature);

        msg_id = esp_mqtt_client_publish(current_client, MY_TOPIC,temp,0,1,0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}
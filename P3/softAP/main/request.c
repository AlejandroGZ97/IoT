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
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO 22         // Pin GPIO para el reloj SCL del bus I2C
#define I2C_MASTER_SDA_IO 21         // Pin GPIO para el dato SDA del bus I2C
#define I2C_MASTER_NUM 0             // I2C port número
#define I2C_MASTER_FREQ_HZ 100000    // Frecuencia de operación del bus I2C
#define AHT10_I2C_ADDR 0x38          // Dirección I2C del sensor AHT10

#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

#define AHT10_CMD_CALIBRATE 0xE1     // Comando de calibración del sensor
#define AHT10_CMD_MEASURE 0xAC       // Comando de medición del sensor
#define AHT10_STATUS_BUSY 0x80       // Estado de ocupado del sensor

static const char *TAG2 = "AHT10";
uint8_t data[6];
char dato[30] = "";
//"{\"Value1\":\"1\"}"

#define SSID ""
#define PASS ""

char* TAG = "Client";

static esp_err_t i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void aht10_calibrate()
{
    uint8_t cmd[3] = {AHT10_CMD_CALIBRATE, 0x08, 0x00};
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, AHT10_I2C_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd_handle, cmd, sizeof(cmd), true);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}

static void aht10_read_data(uint8_t* data)
{
    uint8_t cmd[3] = {AHT10_CMD_MEASURE, 0x33, 0x00};
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, AHT10_I2C_ADDR << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd_handle, cmd, sizeof(cmd), true);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);

    vTaskDelay(75 / portTICK_PERIOD_MS); // Esperar 75 ms

    cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, AHT10_I2C_ADDR << 1 | I2C_MASTER_READ, true);
    i2c_master_read(cmd_handle, data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}

static float aht10_calculate_temperature(const uint8_t *data)
{
    int32_t rawTemperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    return (rawTemperature * 200.0 / 1048576.0) - 50.0;
}

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

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
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

static void rest_post()
{
    esp_http_client_config_t config_post = {
        .url = "http://maker.ifttt.com/trigger/sensor_temperatura/json/with/key/",
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);
    
    esp_http_client_set_post_field(client, dato, strlen(dato));
    
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
    }
    
    esp_http_client_cleanup(client);
}

void app_main(void)
{
    float temperature;
     
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    
    printf("WIFI was initiated ...........\n\n");

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG2, "Inicializado I2C master");
    aht10_calibrate();

    aht10_read_data(data);
    temperature = aht10_calculate_temperature(data);
    ESP_LOGI(TAG, "Temperatura: %.2f °C", temperature);
    sprintf(dato,"{ \"Value1\" : \"%.2f\"}",temperature);

    rest_post();

    while (0)
    {
        aht10_read_data(data);
        temperature = aht10_calculate_temperature(data);
        ESP_LOGI(TAG, "Temperatura: %.2f °C", temperature);
        sprintf(dato,"Temperatura : %.2f ",temperature);
        if (temperature > 25)
        {
            rest_post();
            vTaskDelay(20000 / portTICK_PERIOD_MS);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

}
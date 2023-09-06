#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/i2c.h"

#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define EXAMPLE_ESP_WIFI_SSID      "P7AKS"
#define EXAMPLE_ESP_WIFI_PASS      "SE_12345678"
#define EXAMPLE_MAX_STA_CONN       10

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

static const char *TAG = "AHT10";
char cad[30] = " ";
///////////////////////////////////////////////////////////////////////////////
//static const char *TAG2 = "ESP32_Client";
// Configuración de la red WiFi

#define IP_DEL_ESP32_SERVIDOR  "192.168.4.1"
#define NOMBRE_RED_WIFI        "P7AKS"
#define CLAVE_RED_WIFI    "SE_12345678"
///////////////////////////////////////////////////////////////////////////////

void myItoa(uint16_t number, char* str, uint8_t base)
{
    char nums[15]={0};
	uint16_t count = 0;
	
	if(number < 1 || base < 2) {
		*str++ = '0';
		*str = 0;
		return;
	}

	while(number) {
		if((number % base) > 9) nums[count++] = (number % base) + '0' + 7;
		else nums[count++] = (number % base) + '0';
		number -= (number % base);
		number /= base;
	}

	while(count > 0) {*str++ = nums[--count];}
	*str = 0;
}

void showTemperature(){
    char tempCad[30]={0};

    int temp=5;
    myItoa(temp,cad,10);
    sprintf(tempCad,"Temperatura actual: %d",temp);
    strcpy(cad,tempCad);
}

static const char *TAG2 = "softAP_WebServer";
uint32_t start;

/* An HTTP GET handler */
static esp_err_t commands_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG2, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG2, "Found URL query => %s", buf);
            char command[5];
            /* Get value of expected key from query string */
            showTemperature();
        }
        free(buf);
    }
    
    extern unsigned char file_start[] asm("_binary_commands_html_start");
    extern unsigned char file_end[] asm("_binary_commands_html_end");
    size_t file_len = file_end - file_start;
    char commandsHtml[file_len];

    memcpy(commandsHtml, file_start, file_len);

    char* htmlUpdatedCommand;
    int htmlFormatted = asprintf(&htmlUpdatedCommand, commandsHtml, cad);

    httpd_resp_set_type(req, "text/html");

    if (htmlFormatted > 0)
    {
        httpd_resp_send(req, htmlUpdatedCommand, file_len);
        free(htmlUpdatedCommand);
    }
    else
    {
        ESP_LOGE(TAG2, "Error updating variables");
        httpd_resp_send(req, htmlUpdatedCommand, file_len);
    }

    return ESP_OK;
}

static const httpd_uri_t p7Commands = {
    .uri       = "/p7",
    .method    = HTTP_GET,
    .handler   = commands_get_handler
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Iniciar el servidor httpd 
    ESP_LOGI(TAG2, "Iniciando el servidor en el puerto: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Manejadores de URI
        ESP_LOGI(TAG2, "Registrando manejadores de URI");
        httpd_register_uri_handler(server, &p7Commands);
        //httpd_register_uri_handler(server, &uri_post);
        return server;
    }

    ESP_LOGI(TAG2, "Error starting server!");
    return NULL;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG2, "estacion "MACSTR" se unio, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG2, "estacion "MACSTR" se desconecto, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

esp_err_t wifi_init_softap(void)
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG2, "Inicializacion de softAP terminada. SSID: %s password: %s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    return ESP_OK;
}


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

static float aht10_calculate_humidity(const uint8_t *data)
{
    int32_t rawHumidity = (data[1] << 12) | (data[2] << 4) | ((data[3] >> 4) & 0x0F);
    return (rawHumidity * 100.0) / 1048576.0;
}



void app_main()
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "Inicializado I2C master");
    httpd_handle_t server = NULL;
    
    gpio_reset_pin(2);
    gpio_set_direction(2,GPIO_MODE_OUTPUT);

    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG2, "init softAP");
    ESP_ERROR_CHECK(wifi_init_softap());

    server = start_webserver();

    aht10_calibrate();

    uint8_t data[6];

    while (1)
    {
        aht10_read_data(data);

        float temperature = aht10_calculate_temperature(data);
        ESP_LOGI(TAG, "Temperatura: %.2f °C", temperature);

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Esperar 2 segundos
    }
}
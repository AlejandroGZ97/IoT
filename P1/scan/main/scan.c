#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "myUart.c"
#include "connect.c"

#define LED_1    GPIO_NUM_5
#define LED_2    GPIO_NUM_18
#define LED_3    GPIO_NUM_19

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

/* Initialize Wi-Fi as sta and set scan method */
static void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "Dispositivo \t\t%d", i);
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        strncpy(cad3, (char *) ap_info[i].ssid, sizeof(((char *)ap_info[i].ssid))*3);
        ESP_LOGI(TAG, "NUEVO SSID \t\t%s", cad3);
        if (i < 5){
            meanRSSI += ((int)ap_info[i].rssi);
        }
        if (strcmp(cad3, cad))
        {
            myRSSI = -1*((int)ap_info[i].rssi);
        }
        
    }
    meanRSSI /= -5;
    
    if (myRSSI < meanRSSI)
        myRSSI = meanRSSI - myRSSI;
    else
        myRSSI = myRSSI - meanRSSI;
}

static void initIO(void)
{
    // code to initialize all IO pins for the assigment
    gpio_reset_pin(LED_1);
    gpio_reset_pin(LED_2);
    gpio_reset_pin(LED_3);
    /* Set LED GPIO as a push/pull output */
    gpio_set_direction(LED_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_3, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    uartInit(PC_UART_PORT,115200,8,0,1, PC_UART_TX_PIN, PC_UART_RX_PIN);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    initIO();

    while(1)
    {
        char cad2[30] ={0};
        gpio_set_level(LED_1,0);
        gpio_set_level(LED_2,0);
        gpio_set_level(LED_3,0);

        wifi_scan();

        uartPuts(PC_UART_PORT,"Nombre de red: ");
        uartGets(PC_UART_PORT,cad);
        uartPuts(PC_UART_PORT," Clave: ");
        uartGets(PC_UART_PORT,cad2);
        uartClrScr(PC_UART_PORT);
        delayMs(500);
        const char *wifi_ssid = cad;
        const char *wifi_pass = cad2;
        connect_to_wifi(wifi_ssid, wifi_pass, cad, cad2);
        if (strcmp(cad,"\0")){
            ESP_LOGI(TAG2, "ESP_WIFI_MODE_STA");
            

            delayMs(15000);
            do{
                if (myRSSI < 25)
                {
                    gpio_set_level(LED_1,1);
                    gpio_set_level(LED_2,1);
                    gpio_set_level(LED_3,1);
                }
                else if (myRSSI < 30)
                {
                    gpio_set_level(LED_1,1);
                    gpio_set_level(LED_2,1);
                }
                else
                    gpio_set_level(LED_1,1);
                
                uartPuts(PC_UART_PORT,"Salir? (y): ");
                uartGets(PC_UART_PORT,cad);
                uartClrScr(PC_UART_PORT);
            }while(strcmp(cad,"y"));
        }
        
        else {
            uartPuts(PC_UART_PORT,"DISPOSITIVO NO VALIDO");
            delayMs(2000);
        }
        meanRSSI = 0;
        myRSSI = 0;
    }
}
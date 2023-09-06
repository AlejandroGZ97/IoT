#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define WIFI_SSID      "CUES_UABC"
#define WIFI_PASS      "@dm!nUABC22"

#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE

static const char *TAG = "scan";
int meanRSSI = 0;
int myRSSI = 0;
char cad[30] ={0};
char cad3[32] ={0};


static const char *TAG2 = "Connection";
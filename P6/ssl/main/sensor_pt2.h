#ifndef SENSOR_PT2_H
#define SENSOR_PT2_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

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

static esp_err_t i2c_master_init();
static void aht10_calibrate();
static void aht10_read_data(uint8_t*);
static float aht10_calculate_temperature(const uint8_t*);

#endif
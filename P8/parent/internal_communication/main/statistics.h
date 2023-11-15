#ifndef STATISTICS_H
#define STATISTICS_H

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#define SIZE 5
#define STATS_SIZE 100

float tempValues[SIZE];
bool statsFlag = false; //bandera para mostrar las estadisticas al tener lleno el arreglo de valores
uint8_t i, j, valuesPos = 0;
char allStats[STATS_SIZE], reqData[12];

float mean();
float median();
float variance(float);

#endif
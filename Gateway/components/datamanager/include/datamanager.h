#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xSemaphore;
extern SemaphoreHandle_t xSemaphoreforSim_Lora;

typedef struct 
{
    int id;
    float temperature;
    float PH;
    float DO;
    float EC;
    float latitude;
    float longitude;
} dataSensor_st;
void decodeHexToData(const char *datauart, dataSensor_st *data);
void encodeDataToHex(char *datauart,int id, float PH, float DO, float EC, float temperature, float latitude, float longitude);
extern dataSensor_st dataforSim;

#endif
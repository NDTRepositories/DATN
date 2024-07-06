#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>
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
float truncateDecimal(float num);
#endif
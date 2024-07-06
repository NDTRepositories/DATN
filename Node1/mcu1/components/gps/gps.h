
#ifndef __GPS_H__
#define __GPS_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define UART_GPS UART_NUM_2
#define UART_GPS_TXD GPIO_NUM_5
#define UART_GPS_RXD GPIO_NUM_4


typedef struct GPS_data_
{
    double latitude;
    double longitude;
    double speed_kmh; 
    double speed_ms;  
    double course;  
    double altitude; 
    int hour;
    int minute;
    int second;
    int day;
    int month;
    int year;
} GPS_data;

typedef struct UTC_time
{
    int hour;
    int minute;
    int second;
} UTCtime;

typedef struct UTC_date
{
    int day;
    int month;
    int year;
} UTCdate;


esp_err_t GPS_init();


GPS_data gps_get_value(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __GPS_H__

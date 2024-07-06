// #ifndef GPS_H
// #define GPS_H
// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "string.h"
// #include "driver/gpio.h"
// #include <stdbool.h>

// #define MY_UART_NUM_2 UART_NUM_2
// #define TXD_PIN_2 (GPIO_NUM_17)
// #define RXD_PIN_2 (GPIO_NUM_16)



// typedef struct {
//     char GPS_Buffer[80];
//     bool isGetData;
//     bool isParseData;
//     char UTCTime[11];
//     char latitude[11];
//     char N_S[2];
//     char longitude[12];
//     char E_W[2];
//     bool isUsefull;
// } GPS_Data;

// extern const unsigned int gpsRxBufferLength;
// extern char gpsRxBuffer[600];
// extern unsigned int ii ;


// void GPS_Init();
// void clrGpsRxBuffer(void);
// void parseGpsBuffer();
// void gpsRead();
// #endif
/**
 * @file gps.h
 * @brief GPS driver header file
 */
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

//GPS模块的串口号、引脚号和相关计算数据


#define UART_GPS UART_NUM_2
#define UART_GPS_TXD (GPIO_NUM_5)
#define UART_GPS_RXD (GPIO_NUM_4)


/// @brief GPS数据结构体
typedef struct GPS_data_
{
    double latitude;
    double longitude;
    double speed_kmh; // 单位：千米每小时
    double speed_ms;  // 单位：米每秒
    double course;  // 航向角，单位：度
    double altitude; // 海拔高度，单位：米
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

/// @brief 初始化 GPS模块
esp_err_t GPS_init();

/// @brief 从 GPS模块读取数据
GPS_data gps_get_value(void);
double convertToDecimalDegrees(double ddmm_mmmmm);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __GPS_H__
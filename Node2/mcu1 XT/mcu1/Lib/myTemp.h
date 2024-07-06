#ifndef LIB_myTemp
#define LIB_myTemp
#include "nvs_flash.h"
#include "nvs.h"

#include <unistd.h>
#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdbool.h>
#include<stdlib.h>
#include <math.h>
#include <string.h>
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_interface.h"



float get_Temp(float res, float beta, float t0);



#endif /* LIB_MYDO_H_ */
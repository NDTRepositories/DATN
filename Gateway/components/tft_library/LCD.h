#ifndef lcd_h
#define lcd_h
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "lcd_com.h"
#include "lcd_lib.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_png.h"
#include "pngle.h"

void ArrowTest(TFT_t * dev, FontxFile *fx, int width, int height, uint8_t ascii[24],int a,int b, uint16_t color);
void PNGTest(TFT_t * dev, char * file, int width, int height);
TickType_t BMPTest(TFT_t * dev, char * file, int width, int height) ;
void listSPIFFS(char * path) ;
esp_err_t mountSPIFFS(char * path, char * label, int max_files) ;

#endif

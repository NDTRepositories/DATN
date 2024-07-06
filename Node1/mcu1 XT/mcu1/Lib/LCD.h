#ifndef lcd_h
#define lcd_h

#include <stdio.h>
#include "../Lib/ili9340.h"
#include "../Lib/fontx.h"
#include "../Lib/decode_png.h"
#include "../Lib/pngle.h"


TickType_t BMPTest(TFT_t * dev, char * file, int width, int height);
void ArrowTest(TFT_t * dev, FontxFile *fx, uint16_t model, int width, int height, uint8_t ascii[24],int a,int b);
void PNGTest(TFT_t * dev, char * file, int width, int height);
void ShowPngImage(TFT_t * dev, char * file, int width, int height, int xpos, int ypos) ;
void listSPIFFS(char * path);
esp_err_t mountSPIFFS(char * path, char * label, int max_files);
#endif


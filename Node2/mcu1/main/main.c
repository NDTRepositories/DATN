#include <stdio.h>
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
#include "driver/gpio.h"
#include <stdlib.h>
#include "spi_flash_mmap.h"
#include "sdkconfig.h"
#include <time.h> 
#include "gps.h"
#include "ili9340.h"
#include "fontx.h"
#include "decode_png.h"
#include "pngle.h"
#include "lcd.h"
#include "sdcard.h"
#include "i2cdev.h"
#include "ds3231.h"
#include "phsensor.h"
#include "ecsensor.h"
#include "ads1115.h"
#include "nvs_interface.h"
#include "dosensor.h"
#include "temperature.h"
#include "bmpfile.h"
#include "datamanager.h"
#include "UartComunication.h"

#define IC_PORT1 1
nvs_handle_t nvsHandle;
char buff[126];
char buff_sd[126];
char name_file[50];
char time_buff[50];
char time_buff1[50];

GPS_data gps_data;
TFT_t lcd;
i2c_dev_t ds3231;
struct tm time_ds3231;
static dataSensor_st datasensorforLCD;

double checkKD = 0;
double checkVD = 0;
double hieukd  = 0;
double hieuvd  = 0;

#define pH_ON_PIN1 GPIO_NUM_35
#define DO_ON_PIN1 GPIO_NUM_36
#define EC_ON_PIN1 GPIO_NUM_37


#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)
static const char *TAG = "MAIN";

void SDcard(void *arg)
{
	esp_err_t err_init = sd_card_intialize();
	strcpy(buff_sd, buff);
	strcat(buff_sd, "\n");
	printf("buff_sd:%s", buff_sd);
	esp_err_t err = sd_write_file(name_file, buff_sd);
	if (err != ESP_OK)
	{
		ESP_LOGI(SD_TAG, "Save data error");
	}
	else
	{
		ESP_LOGI(SD_TAG, "Save data success");
	}
	if (err_init == ESP_OK)
	{
		sd_deinitialize();
	}

	ESP_LOGI(SD_TAG,"END SDcard\n");
	vTaskDelete(NULL);
}

dataSensor_st Task_GetDataBkres(dataSensor_st data, int id, nvs_handle_t nvsHandle)
{
    data.id = id;

	data.temperature = get_Temp(10000.0,3950.0,298.15);
	ESP_LOGI(TAG, "Temperature  :%f \n", data.temperature);
	vTaskDelay(1500/portTICK_PERIOD_MS);

    gpio_set_level(pH_ON_PIN1, 1);	
	data.PH = get_pH(nvsHandle, 3300.0, 26400.0);
	ESP_LOGI(TAG, "pH :%f\n", data.PH);
	vTaskDelay(1000/portTICK_PERIOD_MS);
    gpio_set_level(pH_ON_PIN1, 0);

	gpio_set_level(EC_ON_PIN1, 1);
	data.EC = EC_get_value(26400.0,3300.0, 26.0);
	ESP_LOGI(TAG, "EC :%f \n",  data.EC);
	vTaskDelay(1000/portTICK_PERIOD_MS);
	gpio_set_level(EC_ON_PIN1, 0);

	gpio_set_level(DO_ON_PIN1, 1);
	data.DO = get_DO(nvsHandle ,3300.0,  26400.0);
	ESP_LOGI(TAG, "DO :%f \n", data.DO);
	vTaskDelay(1000/portTICK_PERIOD_MS);
	gpio_set_level(DO_ON_PIN1, 0);

    gps_data = gps_get_value();

	data.latitude = gps_data.latitude;
	ESP_LOGI(TAG, "Latitude :%f \n", data.latitude);
	vTaskDelay(1000/portTICK_PERIOD_MS);

	data.longitude = gps_data.longitude;
	ESP_LOGI(TAG, "Longitude :%f \n", data.longitude);
	vTaskDelay(1000/portTICK_PERIOD_MS);

	char databkres[100];
	encodeDataToHex(databkres,data.id,truncateDecimal(data.PH), truncateDecimal(data.DO), truncateDecimal(data.EC), truncateDecimal(data.temperature), data.latitude, data.longitude);
	ESP_LOGI(TAG, "Data :%s \n", databkres);
	int len = strlen(databkres);
	uart_send_data(databkres, len);
	dataSensor_st dataforsdcard = data;

	return dataforsdcard;
}


void SavedataintoSdcard(void *pvParameters)
{   
	dataSensor_st datasensor;
    dataSensor_st sdcard = Task_GetDataBkres(datasensor, 1, nvsHandle);
	datasensorforLCD = sdcard;
	hieukd = fabs(checkKD - gps_data.longitude); hieuvd =fabs(checkVD -gps_data.latitude); 
	if(hieukd <= 0.000165 && hieuvd <= 0.000165)
	{
	sprintf(buff, "%s|%.2f|%.2f|%.2f|%.2f", time_buff, sdcard.temperature, sdcard.PH, sdcard.DO, sdcard.EC);
	}
	else
	{
    sprintf(buff, "%s|%.2f|%.2f|%.2f|%.2f|%f|%f", time_buff, sdcard.temperature, sdcard.PH, sdcard.DO, sdcard.EC,sdcard.latitude,sdcard.longitude);
	}
    checkKD = gps_data.longitude; checkVD = gps_data.latitude;
    xTaskCreatePinnedToCore(SDcard, "SDcard", 2048*2, NULL,5, NULL, tskNO_AFFINITY);
	vTaskDelete(NULL);
}

void ILI9341(void *pvParameters)
{
	FontxFile fx16G[2];
	FontxFile fx24G[2];
	FontxFile fx32G[2];
	FontxFile fx32L[2];
	InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
	InitFontx(fx32L,"/spiffs/LATIN32B.FNT",""); // 16x32Dot Latinc

	FontxFile fx16M[2];
	FontxFile fx24M[2];
	FontxFile fx32M[2];
	InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
	InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
	InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo
    uint16_t model = 0x7735;
	lcdInit(&lcd, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

	char file[32];
	lcdFillScreen(&lcd,BLACK);
	uint8_t ascii[24] ;	
	uint8_t asciiph[24] ;	
	uint8_t asciitemp[24] ;	
	uint8_t asciido[24] ;	
	uint8_t asciiec[24] ;	
	strcpy(file, "/images/ec.png");
	PNGTest(&lcd, file, 25, 110);
	strcpy(file, "/images/do.png");
	PNGTest(&lcd, file, 25, 170);
	strcpy(file, "/images/temp.png");
	PNGTest(&lcd, file, 25, 290);
	strcpy(file, "/images/ph.png");
	PNGTest(&lcd, file, 25, 230);

	while(1) {
		
		// strcpy(file, "/images/san.bmp");
        // BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
        // WAIT;
        ds3231_get_time(&ds3231, &time_ds3231);
		sprintf(time_buff1,"%02d:%02d:%02d", time_ds3231.tm_hour, time_ds3231.tm_min, time_ds3231.tm_sec);
		lcdDrawFillRect(&lcd, 0, 0, 160, 32, BLACK);

		strncpy((char *)ascii, time_buff1, sizeof(ascii) - 1);
		ascii[sizeof(ascii) - 1] = '\0'; // Đảm bảo kết thúc chuỗi
		ArrowTest(&lcd, fx24G, model, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,17,30 );


		lcdDrawFillRect(&lcd, 32, 32, 160, 64, BLACK);
		sprintf((char *)asciiec, "%.2f ms/cm",datasensorforLCD.EC);
		ArrowTest(&lcd, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciiec,30,65 );


		lcdDrawFillRect(&lcd, 32, 64, 160, 96, BLACK);
		sprintf((char *)asciido, "%.2f mg/l",datasensorforLCD.DO);
		ArrowTest(&lcd, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciido,30,95 );



		lcdDrawFillRect(&lcd, 32, 96, 160, 128, BLACK);
		sprintf((char *)asciiph, "%.2f ",datasensorforLCD.PH);
		ArrowTest(&lcd, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciiph,30,125 );




		lcdDrawFillRect(&lcd, 32, 128, 160, 160, BLACK);		
		sprintf((char *)asciitemp, "%.2f oC",datasensorforLCD.temperature);
		ArrowTest(&lcd, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciitemp,30,155 );	
		

		vTaskDelay(800/portTICK_PERIOD_MS);
					
	
	} 
	vTaskDelete(NULL);
}

void ds3231_task(void *pvParameter)
{
	while (1)
	{
	ds3231_get_time(&ds3231, &time_ds3231);
    sprintf(time_buff,"%04d|%02d|%02d|%02d|%02d|%02d",time_ds3231.tm_year, time_ds3231.tm_mon, time_ds3231.tm_mday, time_ds3231.tm_hour, time_ds3231.tm_min, time_ds3231.tm_sec);
	printf("%04d-%02d-%02d %02d:%02d:%02d\n", time_ds3231.tm_year, time_ds3231.tm_mon , time_ds3231.tm_mday, time_ds3231.tm_hour, time_ds3231.tm_min, time_ds3231.tm_sec);
	sprintf(name_file,"log%02d%02d", time_ds3231.tm_mon,time_ds3231.tm_mday);
	xTaskCreatePinnedToCore(SavedataintoSdcard, "SaveDataintoSDcard", 2048*2, NULL, 1, NULL, tskNO_AFFINITY);
	vTaskDelay(60000/portTICK_PERIOD_MS);

	}
	vTaskDelete(NULL);
}


void app_main(void)
{
    uart_init(9,10);

	ESP_ERROR_CHECK(i2cdev_init());
	GPS_init();

	ESP_LOGI(TAG, "Initialize NVS");
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK( err );
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_err_t ret;
	ret = mountSPIFFS("/spiffs", "storage0", 10);
	if (ret != ESP_OK) return;
	listSPIFFS("/spiffs/");
	ret = mountSPIFFS("/icons", "storage1", 10);
	if (ret != ESP_OK) return;
	listSPIFFS("/icons/");
	ret = mountSPIFFS("/images", "storage2", 14);
	if (ret != ESP_OK) return;
	listSPIFFS("/images/");
	

	ESP_LOGI(TAG, "Disable Touch Contoller");
	int XPT_MISO_GPIO = -1;
	int XPT_CS_GPIO = -1;
	int XPT_IRQ_GPIO = -1;
	int XPT_SCLK_GPIO = -1;
	int XPT_MOSI_GPIO = -1;
	spi_master_init(&lcd,CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO,CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);
	
	esp_rom_gpio_pad_select_gpio(pH_ON_PIN1);
	gpio_set_direction(pH_ON_PIN1, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(DO_ON_PIN1);
	gpio_set_direction(DO_ON_PIN1, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(EC_ON_PIN1);
	gpio_set_direction(EC_ON_PIN1, GPIO_MODE_OUTPUT);
  
    init_param_pH(nvsHandle);
    EC_init_param(nvsHandle);
	init_param_DO(nvsHandle);
    
	ds3231_init_desc(&ds3231, IC_PORT1, CONFIG_DS3231_I2C_MASTER_SDA,CONFIG_DS3231_I2C_MASTER_SCL);
	
	// time_ds3231.tm_year = 2024;
	// time_ds3231.tm_mon = 6;
	// time_ds3231.tm_mday= 1;
	// time_ds3231.tm_hour = 00;
	// time_ds3231.tm_min = 05;
	// time_ds3231.tm_sec = 10;
	// ds3231_set_time(&ds3231, &time_ds3231);
	xTaskCreatePinnedToCore(ILI9341, "ILI9341", 1024*6, NULL, 2, NULL, tskNO_AFFINITY);           
    xTaskCreatePinnedToCore(ds3231_task, "ds3231_task", 2048 * 2, NULL, 4, NULL, tskNO_AFFINITY); 
}

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
#include "esp_spi_flash.h"
#include "sdkconfig.h"
#include <time.h> 
#include "../Lib/GPS.h"
#include "../Lib/ili9340.h"
#include "../Lib/fontx.h"
#include "../Lib/decode_png.h"
#include "../Lib/pngle.h"
#include "../Lib/LCD.h"
#include "../Lib/SDCard.h"
#include "../Lib/i2cdev.h"
#include "../Lib/ds3231.h"
#include "../Lib/mypH.h"
#include "../Lib/myEC.h"
#include "../Lib/ADS1115.h"
#include "../Lib/nvs_interface.h"
#include "../Lib/myDO.h"
#include "../Lib/myTemp.h"
#include "../Lib/bmpfile.h"
#include "../Lib/datamanager.h"
#include "../Lib/UartComunication.h"
nvs_handle_t nvsHandle;
TaskHandle_t test_ads1115;
TaskHandle_t test_ds3231;
TaskHandle_t p_sd_card;
TaskHandle_t get_data;
TaskHandle_t send_data;

volatile char buff[126];
volatile char buff_sd[126];
volatile char name_file[50];
volatile char time_buff[50];
volatile char time_buff1[50];
volatile dataSensor_st databkres;
GPS_data gps_data;
TFT_t dev;
i2c_dev_t dev1;
struct tm time1 ;
double checkKD = 0;
double checkVD = 0;
double hieukd ;
double hieuvd ;

double checkDO = 0;
double hieuDO ;

double checkpH = 0;
double hieuPH ;

double checkEC = 0;
double hieuEC ;

double checkTemp = 0;
double hieuTemp ;



#define pH_ON_PIN1 GPIO_NUM_35
#define DO_ON_PIN1 GPIO_NUM_36
#define EC_ON_PIN1 GPIO_NUM_37


#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)
static const char *TAG = "MAIN";

static void GPS(void *pvParameters)
{
	
	gps_data = gps_get_value();	
	
	/* code *///0.000145
    vTaskDelete(NULL);
}

void sd_task(void *arg)
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
	printf("END sd_task\n");
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
	lcdInit(&dev, model, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

	char file[32];
	lcdFillScreen(&dev,BLACK);
	uint8_t ascii[24] ;	
	uint8_t asciiph[24] ;	
	uint8_t asciitemp[24] ;	
	uint8_t asciido[24] ;	
	uint8_t asciiec[24] ;	
	strcpy(file, "/images/ec.png");
	PNGTest(&dev, file, 25, 110);
	strcpy(file, "/images/do.png");
	PNGTest(&dev, file, 25, 170);
	strcpy(file, "/images/temp.png");
	PNGTest(&dev, file, 25, 290);
	strcpy(file, "/images/ph.png");
	PNGTest(&dev, file, 25, 230);

	while(1) {
				
        ds3231_get_time(&dev1, &time1);
		sprintf(time_buff1,"%02d:%02d:%02d", time1.tm_hour, time1.tm_min, time1.tm_sec);
		lcdDrawFillRect(&dev, 0, 0, 160, 32, BLACK);

		strncpy((char *)ascii, time_buff1, sizeof(ascii) - 1);
		ascii[sizeof(ascii) - 1] = '\0'; // Đảm bảo kết thúc chuỗi
		ArrowTest(&dev, fx24G, model, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,17,30 );
        
		hieuEC = fabs(checkEC - databkres.EC);  
		if(hieuEC == 0){
		lcdDrawFillRect(&dev, 32, 32, 160, 64, BLACK);
		sprintf((char *)asciiec, " %.2f g/l",databkres.EC);
		ArrowTest(&dev, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciiec,30,65 );
		}
		checkEC = 1000;

		hieuDO = fabs(checkDO - databkres.DO);
		if(hieuDO == 0){
		lcdDrawFillRect(&dev, 32, 64, 160, 96, BLACK);
		sprintf((char *)asciido, " %.2f mg/l",databkres.DO);
		ArrowTest(&dev, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciido,30,95 );
		}
		checkDO = 1000;

        hieuPH = fabs(checkpH - databkres.PH);
		if(hieuPH == 0){
		lcdDrawFillRect(&dev, 32, 96, 160, 128, BLACK);
		sprintf((char *)asciiph, " %.2f ",databkres.PH);
		ArrowTest(&dev, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciiph,30,125 );
		}
        checkpH = 1000;

		hieuTemp = fabs(checkTemp - databkres.temperature);
		if(hieuTemp == 0){
		lcdDrawFillRect(&dev, 32, 128, 160, 160, BLACK);		
		sprintf((char *)asciitemp, " %.2f oC",databkres.temperature);
		ArrowTest(&dev, fx16G, model, CONFIG_WIDTH, CONFIG_HEIGHT,asciitemp,30,155 );	
		}
        checkTemp = 1000;	
				
		vTaskDelay(905/portTICK_PERIOD_MS);	
	
	} 
 }

void task_get_data(void *arg){
    databkres.id = 1;
   	databkres.temperature = get_Temp(10000.0,3950.0,298.15);
	printf("Temp :%f \n", databkres.temperature);
	checkTemp = databkres.temperature;
    vTaskDelay(2000/portTICK_PERIOD_MS);

	gpio_set_level(pH_ON_PIN1, 1);
	databkres.PH = get_pH(nvsHandle, 3300.0, 26400.0);
	printf("pH :%f\n", databkres.PH);
	checkpH = databkres.PH;
	gpio_set_level(pH_ON_PIN1, 0);
    vTaskDelay(2000/portTICK_PERIOD_MS);

	gpio_set_level(EC_ON_PIN1, 1);
    databkres.EC = EC_get_value(26400.0,3300.0, 26.0);
	printf("EC :%f \n",  databkres.EC);
	checkEC = databkres.EC;
    gpio_set_level(EC_ON_PIN1, 0);
	vTaskDelay(2000/portTICK_PERIOD_MS);

    gpio_set_level(DO_ON_PIN1, 1);
	databkres.DO = get_DO(nvsHandle ,3300.0,  26400.0);
	printf("DO :%f \n", databkres.DO);
	checkDO = databkres.DO;
    gpio_set_level(DO_ON_PIN1, 0);
	vTaskDelay(2000/portTICK_PERIOD_MS);

	xTaskCreate(&GPS, "GPS", 4096, NULL, 5, NULL);  
    databkres.latitude = gps_data.latitude;
	databkres.longitude = gps_data.longitude;
	char data[100];
	encodeDataToHex(data,databkres.id,truncateDecimal(databkres.PH),truncateDecimal(databkres.DO),truncateDecimal(databkres.EC),truncateDecimal(databkres.temperature),databkres.latitude,databkres.longitude);
	int len =strlen(data);
	ESP_LOGW(TAG, "Data uart Bkres: %s", data);
	uart_send_data(data,len);


	hieukd = fabs(checkKD - gps_data.longitude); hieuvd =fabs(checkVD -gps_data.latitude); 
	if(hieukd <= 0.000165 && hieuvd <= 0.000165)
	{
	sprintf(buff, "%s|%.2f|%.2f|%.2f|%.2f", time_buff, databkres.temperature, databkres.PH, databkres.DO, databkres.EC);}
	else{
    sprintf(buff, "%s|%.2f|%.2f|%.2f|%.2f|%f|%f", time_buff, databkres.temperature, databkres.PH, databkres.DO, databkres.EC,gps_data.latitude,gps_data.longitude);
	}
    checkKD = gps_data.longitude; checkVD = gps_data.latitude;
	xTaskCreatePinnedToCore(sd_task, "sd_task", 2048*2, NULL,5, p_sd_card, tskNO_AFFINITY);  
    vTaskDelete(NULL);
}

void ds3231_task(void *pvParameter)
{
	// i2c_dev_t dev;
	// if (ds3231_init_desc(&dev, 0, 7, 6) != ESP_OK) {
	// 	//ESP_LOGE(pcTaskGetName(0), "Could not init device descriptor.");
	// 	while (1) { vTaskDelay(1000); }
	// }
	// struct tm time ;
	// 		// time.tm_year = 2024;
	// 		// time.tm_mon = 6;
	// 		// time.tm_mday= 1;
	// 		// time.tm_hour = 00;
	// 		// time.tm_min = 05;
	// 		// time.tm_sec = 10;
	// 		// ds3231_set_time(&dev, &time);
	// ds3231_get_time(&dev, &time);
	ds3231_get_time(&dev1, &time1);
    sprintf(time_buff,"%04d|%02d|%02d|%02d|%02d|%02d",time1.tm_year, time1.tm_mon, time1.tm_mday, time1.tm_hour, time1.tm_min, time1.tm_sec);
	printf("%04d-%02d-%02d %02d:%02d:%02d\n", time1.tm_year, time1.tm_mon , time1.tm_mday, time1.tm_hour, time1.tm_min, time1.tm_sec);
	sprintf(name_file,"log%02d%02d", time1.tm_mon,time1.tm_mday);
	xTaskCreatePinnedToCore(task_get_data, "get_data", 2048*2, NULL, 1, &get_data, tskNO_AFFINITY);
	vTaskDelete(NULL);

}


void app_main(void)
{
///////////////////////////////////////////////////////    INIT	
	//nvs_flash_init();
	////DS3231
	ESP_ERROR_CHECK(i2cdev_init());
	////UART 
	uart_init(9,10);
	////GPS
	GPS_init();
	////LCD

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
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, 
	CONFIG_RESET_GPIO, CONFIG_BL_GPIO, XPT_MISO_GPIO, XPT_CS_GPIO, XPT_IRQ_GPIO, XPT_SCLK_GPIO, XPT_MOSI_GPIO);
	
	

    ////ADS1115
    esp_rom_gpio_pad_select_gpio(pH_ON_PIN1);
	gpio_set_direction(pH_ON_PIN1, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(DO_ON_PIN1);
	gpio_set_direction(DO_ON_PIN1, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(EC_ON_PIN1);
	gpio_set_direction(EC_ON_PIN1, GPIO_MODE_OUTPUT);
  
    init_param_pH(nvsHandle);
    EC_init_param(nvsHandle);
	init_param_DO(nvsHandle);
    
	ds3231_init_desc(&dev1, 0, 7, 6);
	
			// time.tm_year = 2024;
			// time.tm_mon = 6;
			// time.tm_mday= 1;
			// time.tm_hour = 00;
			// time.tm_min = 05;
			// time.tm_sec = 10;
			// ds3231_set_time(&dev, &time);
	
///////////////////////////////////////////////////////////// END INIT

    // Luồng vận hành
	

	//xTaskCreate(ILI9341, "ILI9341", 1024*6, NULL, 2, NULL);           
	//vTaskDelay(pdMS_TO_TICKS(6000));                                      //#LCD
    //xTaskCreatePinnedToCore(sd_task, "sd_task", 2048*2, NULL,5, p_sd_card, tskNO_AFFINITY);                 //#SDCARD
    //xTaskCreate(&GPS, "GPS", 4096, NULL, 5, NULL);                                                          //#GPS
    //xTaskCreatePinnedToCore(ds3231_task, "ds3231_task", 2048 * 2, NULL, 4, &test_ds3231, tskNO_AFFINITY);     //#DS3231

	xTaskCreate(ILI9341, "ILI9341", 1024*6, NULL, 2, NULL);  
    //pH_Calib(3300.0, 26400.0, nvsHandle, "storage", pH_CALIB_9);
	//EC_calibrate(nvsHandle,3300.0, 26400.0, "storage", 12.88, EC_calib_hight_12_88, 27.0);
	// DO_Calib(3300.0, 26400.0, nvsHandle,"storage", do_calib_0);
	// void ILI9341(NULL);    
	//xTaskCreate(ILI9341, "ILI9341", 1024*6, NULL, 2, NULL);   
	while (1)
	{
    xTaskCreatePinnedToCore(ds3231_task, "ds3231_task", 2048 * 2, NULL, 4, &test_ds3231, tskNO_AFFINITY); 
	// xTaskCreate(ILI9341, "ILI9341", 1024*6, NULL, 2, NULL);               
	// xTaskCreatePinnedToCore(send_data_task, "send_data_task", 2048 * 2, NULL, 4, &send_data, tskNO_AFFINITY);
    // xTaskCreate(&GPS, "GPS", 4096, NULL, 5, NULL);             
	vTaskDelay(pdMS_TO_TICKS(60000));
	ESP_LOGI(TAG, "--------------------------ABC----------------------------\n");
	}
	// while(1){

	// data.temp = get_Temp(10000.0,3950.0,298.15);
	// printf("Temp :%f \n", data.temp);
	// vTaskDelay(2000/portTICK_PERIOD_MS);

	// gpio_set_level(pH_ON_PIN1, 1);
	// data.pH = get_pH(nvsHandle, 3300.0, 26400.0);
	// printf("pH :%f\n", data.pH);
	// gpio_set_level(pH_ON_PIN1, 0);
	// vTaskDelay(2000/portTICK_PERIOD_MS);

	// gpio_set_level(EC_ON_PIN1, 1);
	// data.EC = EC_get_value(26400.0,3300.0, 26.0);
	// printf("EC :%f \n",  data.EC);
	// gpio_set_level(EC_ON_PIN1, 0);
	// vTaskDelay(2000/portTICK_PERIOD_MS);


	// gpio_set_level(DO_ON_PIN1, 1);
	// data.DO = get_DO(nvsHandle ,3300.0,  26400.0);
	// printf("DO :%f \n", data.DO);
	// gpio_set_level(DO_ON_PIN1, 0);
	// vTaskDelay(2000/portTICK_PERIOD_MS);

	//  }
	
    

}

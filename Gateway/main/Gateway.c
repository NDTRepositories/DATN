#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "lora.h"
#include "datamanager.h"
#include "http.h"
#include "gps.h"  

#include "esp_err.h"
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
#include "decode_png.h"
#include "ds3231.h"
#include "datamanager.h"
#include "i2cdev.h"
#include <freertos/event_groups.h>
#include "lcd_com.h"
#include "lcd_lib.h"
#include "fontx.h"
#include "bmpfile.h"
#include "pngle.h"
#include "LCD.h"

#if CONFIG_INTERFACE_I2S
#define INTERFACE INTERFACE_I2S
#elif CONFIG_INTERFACE_GPIO
#define INTERFACE INTERFACE_GPIO
#elif CONFIG_INTERFACE_REG
#define INTERFACE INTERFACE_REG
#endif

char *LCD_Connected = "CONNECTED";
char *LCD_Disconnected = "DISCONNECTED";

#if CONFIG_ILI9341
#include "ili9341.h"
#define DRIVER "ILI9341"
#define INIT_FUNCTION(a, b, c, d, e) ili9341_lcdInit(a, b, c, d, e)

#elif CONFIG_ILI9486
#include "ili9486.h"
#define DRIVER "ILI9486"
#define INIT_FUNCTION(a, b, c, d, e) ili9486_lcdInit(a, b, c, d, e)

#endif

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)
#define DS3231_I2C_PORT 0
#define BIT_0 (1 << 0)


TFT_t dev;
i2c_dev_t ds3231;

static const char *TAG = "MAIN";
struct tm time_ds3231 ;

FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];

FontxFile fx16M[2];
FontxFile fx24M[2];
FontxFile fx32M[2];

char time_buff[50];
int nhay1 =0;
int nhay2 =0;
int check_Node1 = 0;
int check_Node2 = 0;

static EventGroupHandle_t xEventGroup;
SemaphoreHandle_t xSemaphore = NULL;
dataSensor_st dataforSim;
void send_data_task_http(void *arg);
void uart_rx_task_http(void *arg);
void Timeout_task_http(void *arg);
void SendGPSGateWaytoServer(float latitude, float longitude,float threshold);

void ManageDatafromUarttoSIM(void *arg)
{
	if(xTaskCreatePinnedToCore(send_data_task_http, "send data uart task", 1024*8, NULL, 32, NULL, tskNO_AFFINITY) == pdPASS)
	{
		xTaskCreatePinnedToCore(uart_rx_task_http, "uart rx task", 1024*6, NULL, 1, NULL, tskNO_AFFINITY);
	    xTaskCreatePinnedToCore(Timeout_task_http, "timeout task",3072, NULL, 3, NULL, tskNO_AFFINITY);
	}
	else
	{
		ESP_LOGI(pcTaskGetName(NULL), "Task send data uart task could not be created");
	}
    vTaskDelete(NULL);
}

void TFT(void *pvParameters)
{
	char file[32];
	lcdFillScreen(&dev,BLACK);
	uint8_t ascii[24] ;	
	

    strcpy(file, "/images/sanlab.bmp");
    BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

	sprintf((char *)ascii, "NODE 1");
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,200,340,WHITE );
	
	sprintf((char *)ascii, "NODE 2");
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,60,340 ,WHITE);

	sprintf((char *)ascii, "CONNECTING");
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,10,300,GREEN );

	sprintf((char *)ascii, "CONNECTING");
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,150,300,GREEN );	

    while (1)
	{
	ds3231_get_time(&ds3231, &time_ds3231);
	sprintf(time_buff,"%02d:%02d:%02d", time_ds3231.tm_hour, time_ds3231.tm_min, time_ds3231.tm_sec);
	lcdDrawFillRect(&dev, 250, 40, 300, 200,BLACK);
	memcpy((char *)ascii, time_buff, sizeof(ascii) - 1);
	ascii[sizeof(ascii) - 1] = '\0';
    ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,250,50,WHITE );

    if(check_Node1 == 1 && nhay1 == 0)
	{
	lcdDrawFillRect(&dev, 150, 240, 190, 480,BLACK);
	sprintf((char *)ascii, LCD_Connected);
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,150,300,GREEN );
	nhay1 = 10;
	}else if(check_Node1 == 2 && nhay1 ==0)
	{
	lcdDrawFillRect(&dev, 150, 240, 190, 480,BLACK);
	sprintf((char *)ascii, LCD_Disconnected);
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,150,270,RED );
	nhay1 =10;
	}

	if(check_Node2 == 1 && nhay2 ==0)
	{
	lcdDrawFillRect(&dev, 10, 240, 50, 480,BLACK);
	sprintf((char *)ascii, LCD_Connected);
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,10,300,GREEN );
	nhay2 =10;		
	}
	else if(check_Node2 == 2 && nhay2 ==0)
	{
	lcdDrawFillRect(&dev, 10, 240, 50, 480,BLACK);
	sprintf((char *)ascii, LCD_Disconnected);
	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,10,270,RED );	
	nhay2 =10;	
	}
	
	vTaskDelay(920 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);

}
void TaskSIM(void *arg)
{
	ESP_LOGI(pcTaskGetName(NULL), " Task SIM waiting for BIT_0");
	EventBits_t uxBits = xEventGroupWaitBits(
		xEventGroup, 
		BIT_0,        
		pdTRUE,       
		pdFALSE,      
		portMAX_DELAY  
	);
	if((uxBits & BIT_0) == BIT_0)
	{
		ESP_LOGI(pcTaskGetName(NULL), " Task SIM received BIT_0");
		ManageDatafromUarttoSIM(NULL);
	}
	vTaskDelete(NULL);
}

void gps_task(void *arg)
{
    const char *TAG = "GPS";
    while (1)
    {
        GPS_data gps_data = gps_get_value();
        ESP_LOGI(TAG, "lat: %f,   lon: %f   \n", gps_data.latitude, gps_data.longitude); 
		xSemaphoreTake(xSemaphore, portMAX_DELAY);
		SendGPSGateWaytoServer(gps_data.latitude, gps_data.longitude,0.000165 ); 
		xSemaphoreGive(xSemaphore);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
	vTaskDelete(NULL);
}  

#if CONFIG_PRIMARY

#define TIMEOUT 100

void task_primary(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		int send_len = sprintf((char *)buf,"Hello World!! %"PRIu32, nowTick);

		lora_send_packet(buf, send_len);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent:[%.*s]", send_len, send_len, buf);

		bool waiting = true;
		TickType_t startTick = xTaskGetTickCount();
		while(waiting) 
		{
			lora_receive(); // put into receive mode
			if(lora_received()) {
				int rxLen = lora_receive_packet(buf, sizeof(buf));
				TickType_t currentTick = xTaskGetTickCount();
				TickType_t diffTick = currentTick - startTick;
				ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
				ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" millisecond", diffTick * portTICK_PERIOD_MS);
				waiting = false;
			}
			TickType_t currentTick = xTaskGetTickCount();
			TickType_t diffTick = currentTick - startTick;
			ESP_LOGD(pcTaskGetName(NULL), "diffTick=%"PRIu32, diffTick);
			if (diffTick > TIMEOUT) {
				ESP_LOGW(pcTaskGetName(NULL), "Response timeout");
				waiting = false;
			}
			vTaskDelay(1); // Avoid WatchDog alerts
		} // end waiting
		vTaskDelay(pdMS_TO_TICKS(5000));
	} // end while
}
#endif // CONFIG_PRIMARY

#if CONFIG_SECONDARY
void task_secondary(void *pvParameters)
{
   static TickType_t nowTick_Node1 = 0;
   static TickType_t nowTick_Node2 = 0;
   static TickType_t lastTick_Node1 = 0;
   static TickType_t lastTick_Node2 = 0;
   static TickType_t CheckFinalNode = 0; 

    char *ack = "ACK";
    int ack_len = strlen(ack);
    char *NACK = "NACK";
    int NACK_len = strlen(NACK);  
    ESP_LOGI(pcTaskGetName(NULL), "Start");
    dataSensor_st dataSensor;
    uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
    while(1) 
    {
        CheckFinalNode = xTaskGetTickCount();
        lora_receive(); // put into receive mode
        if(lora_received()) 
        {
            lora_receive_packet(buf, sizeof(buf));
            decodeHexToData((const char *)buf, &dataSensor);

            dataforSim = dataSensor;

            ESP_LOGI(pcTaskGetName(NULL),"Device %d PH =%.2f, DO =%.2f, EC = %.2f, TEMP = %.2f, LATI = %.6f, LONGTI = %.6f\n",dataSensor.id, dataSensor.PH, dataSensor.DO, dataSensor.EC, dataSensor.temperature, dataSensor.latitude, dataSensor.longitude);
            ESP_LOGI(pcTaskGetName(NULL), "Task Lora setting BIT_0");

            xEventGroupSetBits(xEventGroup, BIT_0);
            if(dataSensor.id == 1)
            {
                lastTick_Node1 = nowTick_Node1;
                nowTick_Node1 = xTaskGetTickCount();
                ESP_LOGI(pcTaskGetName(NULL), "Node 1 Connected\n");

                // Check if Node 2 has been disconnected
                if (nowTick_Node1 > lastTick_Node2 && nowTick_Node1 - lastTick_Node2 > 30000)
                {
                    lora_send_packet((uint8_t*)NACK, NACK_len);
                    ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", NACK_len);
                    ESP_LOGI(pcTaskGetName(NULL), "Node 2 Disconnected \n");
                    nhay2 = 0;
                    check_Node2 = 2;
                }
                lora_send_packet((uint8_t*)ack, ack_len);  
                ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", ack_len);
                check_Node1 = 1;
                nhay1 = 0;
            }

            if(dataSensor.id == 2)
            {
                lastTick_Node2 = nowTick_Node2;
                nowTick_Node2 = xTaskGetTickCount();
                ESP_LOGI(pcTaskGetName(NULL), "Node 2 Connected\n");
                // Check if Node 1 has been disconnected
                if (nowTick_Node2 > lastTick_Node1 && nowTick_Node2 - lastTick_Node1 > 30000)
                {
                    lora_send_packet((uint8_t*)NACK, NACK_len);
                    ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", NACK_len);
                    ESP_LOGI(pcTaskGetName(NULL), "Node 1 Disconnected\n");
                    nhay1 = 0;
                    check_Node1 = 2;
                }
                lora_send_packet((uint8_t*)ack, ack_len);
                ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", ack_len);                                                
                check_Node2 = 1;
                nhay2 = 0;
            }
            // Check if Node 1 or Node 2 is disconnected

            ESP_LOGE(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
            ESP_LOGE(pcTaskGetName(NULL), "lora packet rssi :%d dbm", lora_packet_rssi());
            ESP_LOGE(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());

            vTaskDelay(1);

        }
        else 
        {
            if(CheckFinalNode - nowTick_Node1 > 10000 && CheckFinalNode - nowTick_Node2 > 10000)
            {
                ESP_LOGI(pcTaskGetName(NULL), "Node 1 Disconnected\n");
                ESP_LOGI(pcTaskGetName(NULL), "Node 2 Disconnected\n");
                nhay1 = 0;
                check_Node1 = 2;
                nhay2 = 0;
                check_Node2 = 2;
            }
        }
        vTaskDelay(1); // Avoid WatchDog alerts
    }
}
#endif // CONFIG_SECONDARY 

void app_main()
{
	if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}
	GPS_init();
    xSemaphore = xSemaphoreCreateMutex();
	if (xSemaphore == NULL)
	{
		ESP_LOGI(pcTaskGetName(NULL),"Failed to create Semaphore\n");
	}
	else
	{
		ESP_LOGI(pcTaskGetName(NULL),"Semaphore created\n");
	}
    xEventGroup = xEventGroupCreate();

    if (xEventGroup == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }	
	ESP_ERROR_CHECK(i2cdev_init());
	ds3231_init_desc(&ds3231, DS3231_I2C_PORT, CONFIG_DS3231_I2C_MASTER_SDA, CONFIG_DS3231_I2C_MASTER_SCL);

	// time_ds3231.tm_year = 2024;
	// time_ds3231.tm_mon = 6;
	// time_ds3231.tm_mday= 6;
	// time_ds3231.tm_hour = 00;
	// time_ds3231.tm_min = 10;
	// time_ds3231.tm_sec = 10;

	// ds3231_set_time(&ds3231, &time_ds3231);

	ESP_LOGI(TAG, "Initialize NVS");
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
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

	lcd_interface_cfg(&dev, INTERFACE);
	INIT_FUNCTION(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
	InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
	InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
	InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
	InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo	
    
#if CONFIG_169MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
	lora_set_frequency(169e6); // 169MHz
#elif CONFIG_433MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
	lora_set_frequency(433e6); // 433MHz
#elif CONFIG_470MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
	lora_set_frequency(470e6); // 470MHz
#elif CONFIG_866MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
	lora_set_frequency(866e6); // 866MHz
#elif CONFIG_915MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
	lora_set_frequency(915e6); // 915MHz
#elif CONFIG_OTHER
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
	long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
	lora_set_frequency(frequency);
#endif

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 9;
#if CONFIF_EXTENDED
	cr = CONFIG_CODING_RATE
	bw = CONFIG_BANDWIDTH;
	sf = CONFIG_SF_RATE;
#endif

	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	//cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	//int bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	//int sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

#if CONFIG_PRIMARY
	xTaskCreate(&task_primary, "PRIMARY", 1024*3, NULL, 5, NULL);
#endif
#if CONFIG_SECONDARY
	xTaskCreatePinnedToCore(&task_secondary, "SECONDARY", 4096, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(&TaskSIM, "TaskSIM", 4096, NULL, 5, NULL, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(TFT, "TFT", 1024*6, NULL, 5, NULL, tskNO_AFFINITY);	
    xTaskCreatePinnedToCore(&gps_task, "gps_task", 1024 * 4, NULL, 5, NULL, tskNO_AFFINITY);

#endif
}



/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

// #include <stdio.h>
// #include <inttypes.h>
// #include <string.h>
// #include <ctype.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "lora.h"

// #if CONFIG_PRIMARY

// #define TIMEOUT 100

// void task_primary(void *pvParameters)
// {
// 	ESP_LOGI(pcTaskGetName(NULL), "Start");
// 	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
// 	while(1) {
// 		TickType_t nowTick = xTaskGetTickCount();
// 		int send_len = sprintf((char *)buf,"Hello World!!Hello World!!Hello World!!Hello World!!");

// #if 0
// 		// Maximum Payload size of SX1276/77/78/79 is 255
// 		memset(&buf[send_len], 0x20, 255-send_len);
// 		send_len = 255;
// #endif

// 		lora_send_packet(buf, send_len);
// 		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent:[%.*s]", send_len, send_len, buf);

// 		bool waiting = true;
// 		TickType_t startTick = xTaskGetTickCount();
// 		while(waiting) {
// 			lora_receive(); // put into receive mode
// 			if(lora_received()) {
// 				int rxLen = lora_receive_packet(buf, sizeof(buf));
// 				TickType_t currentTick = xTaskGetTickCount();
// 				TickType_t diffTick = currentTick - startTick;
// 				ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
// 				ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" millisecond", diffTick * portTICK_PERIOD_MS);
// 				waiting = false;
// 			}
// 			TickType_t currentTick = xTaskGetTickCount();
// 			TickType_t diffTick = currentTick - startTick;
// 			ESP_LOGD(pcTaskGetName(NULL), "diffTick=%"PRIu32, diffTick);
// 			if (diffTick > TIMEOUT) {
// 				ESP_LOGW(pcTaskGetName(NULL), "Response timeout");
// 				waiting = false;
// 			}
// 			vTaskDelay(1); // Avoid WatchDog alerts
// 		} // end waiting
// 		vTaskDelay(pdMS_TO_TICKS(5000));
// 	} // end while
// }
// #endif // CONFIG_PRIMARY

// #if CONFIG_SECONDARY
// void task_secondary(void *pvParameters)
// {
// 	ESP_LOGI(pcTaskGetName(NULL), "Start");
// 	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
// 	while(1) {
// 		lora_receive(); // put into receive mode
// 		if(lora_received()) {
// 			int rxLen = lora_receive_packet(buf, sizeof(buf));
// 			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
// 			char *ack = "ACK";
// 			int ack_len = strlen(ack);
// 			vTaskDelay(1);
// 			lora_send_packet((uint8_t*)ack, ack_len);
// 			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", ack_len);
// 			ESP_LOGE(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
//  			ESP_LOGE(pcTaskGetName(NULL), "lora packet rssi :%d dbm", lora_packet_rssi());
//  			ESP_LOGE(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
// 		}
// 		vTaskDelay(1); // Avoid WatchDog alerts
// 	}
// }
// #endif // CONFIG_SECONDARY

// void app_main()
// {
// 	if (lora_init() == 0) {
// 		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
// 		while(1) {
// 			vTaskDelay(1);
// 		}
// 	}

// #if CONFIG_169MHZ
// 	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
// 	lora_set_frequency(169e6); // 169MHz
// #elif CONFIG_433MHZ
// 	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
// 	lora_set_frequency(433e6); // 433MHz
// #elif CONFIG_470MHZ
// 	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
// 	lora_set_frequency(470e6); // 470MHz
// #elif CONFIG_866MHZ
// 	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
// 	lora_set_frequency(866e6); // 866MHz
// #elif CONFIG_915MHZ
// 	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
// 	lora_set_frequency(915e6); // 915MHz
// #elif CONFIG_OTHER
// 	ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
// 	long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
// 	lora_set_frequency(frequency);
// #endif

// 	lora_enable_crc();

// 	int cr = 1;
// 	int bw = 7;
// 	int sf = 12;
// #if CONFIF_EXTENDED
// 	cr = CONFIG_CODING_RATE
// 	bw = CONFIG_BANDWIDTH;
// 	sf = CONFIG_SF_RATE;
// #endif

// 	lora_set_coding_rate(cr);
// 	//lora_set_coding_rate(CONFIG_CODING_RATE);
// 	//cr = lora_get_coding_rate();
// 	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

// 	lora_set_bandwidth(bw);
// 	//lora_set_bandwidth(CONFIG_BANDWIDTH);
// 	//int bw = lora_get_bandwidth();
// 	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

// 	lora_set_spreading_factor(sf);
// 	//lora_set_spreading_factor(CONFIG_SF_RATE);
// 	//int sf = lora_get_spreading_factor();
// 	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

// #if CONFIG_PRIMARY
// 	xTaskCreate(&task_primary, "PRIMARY", 1024*3, NULL, 5, NULL);
// #endif
// #if CONFIG_SECONDARY
// 	xTaskCreate(&task_secondary, "SECONDARY", 1024*3, NULL, 5, NULL);
// #endif
// }

// void send_data_task(void *arg);
// void uart_rx_task(void *arg);
// void Timeout_task(void *arg);

// void ManageDatafromUarttoSIM(void *arg){
//     xTaskCreatePinnedToCore(uart_rx_task, "uart rx task", 4096*2, NULL, 1, NULL, tskNO_AFFINITY);
// 	xTaskCreatePinnedToCore(send_data_task, "send data uart task", 4096*2, NULL, 32, NULL, tskNO_AFFINITY);
// 	xTaskCreatePinnedToCore(Timeout_task, "timeout task", 4096*2, NULL, 3, NULL, tskNO_AFFINITY);
// 	vTaskDelete(NULL);

// }


// #if CONFIG_INTERFACE_I2S
// #define INTERFACE INTERFACE_I2S
// #elif CONFIG_INTERFACE_GPIO
// #define INTERFACE INTERFACE_GPIO
// #elif CONFIG_INTERFACE_REG
// #define INTERFACE INTERFACE_REG
// #endif


// #if CONFIG_ILI9341
// #include "ili9341.h"
// #define DRIVER "ILI9341"
// #define INIT_FUNCTION(a, b, c, d, e) ili9341_lcdInit(a, b, c, d, e)

// #elif CONFIG_ILI9486
// #include "ili9486.h"
// #define DRIVER "ILI9486"
// #define INIT_FUNCTION(a, b, c, d, e) ili9486_lcdInit(a, b, c, d, e)

// #endif

// #define INTERVAL 400
// #define WAIT vTaskDelay(INTERVAL)
// TFT_t dev;
// static const char *TAG = "MAIN";

// FontxFile fx16G[2];
// FontxFile fx24G[2];
// FontxFile fx32G[2];

// FontxFile fx16M[2];
// FontxFile fx24M[2];
// FontxFile fx32M[2];



// void TFT(void *pvParameters)
// {
// 	char file[32];
// 	lcdFillScreen(&dev,BLACK);
// 	uint8_t ascii[24] ;	
	
// 	float ec =5.4;
	


//     strcpy(file, "/images/sanlab.bmp");
//     BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);

// 	// 	lcdDrawFillRect(&dev, 0, 0, 240,240 , RED);
// 	// WAIT;
// 	// lcdDrawFillRect(&dev, 0, 240, 0,240 , BLUE);
// 	//WAIT;
// 	// // lcdDrawFillRect(&dev, 0, 0, 240,240 , RED);
// 	// // WAIT;
// 	// // lcdDrawFillRect(&dev, 0, 0, 240,240 , RED);
// 	// // WAIT;



	
// 	sprintf((char *)ascii, "20:11:23");
// 	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,250,50,WHITE );
	

// 	sprintf((char *)ascii, "NODE 1");
// 	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,200,340,WHITE );
	

// 	sprintf((char *)ascii, "CONNECTING");
// 	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,150,300,GREEN );
	

// 	sprintf((char *)ascii, "NODE 2");
// 	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,60,340 ,WHITE);
	

// 	sprintf((char *)ascii, "CONNECTING");
// 	ArrowTest(&dev, fx32M, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,10,300,GREEN );
	
// 	// sprintf((char *)ascii, "BKRES");
// 	// ArrowTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT,ascii,0,150 );
// 	// WAIT;

// vTaskDelete(NULL);

// }

// void app_main()
// {
// 	// Initialize NVS
// 	ESP_LOGI(TAG, "Initialize NVS");
// 	esp_err_t err = nvs_flash_init();
// 	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
// 		// NVS partition was truncated and needs to be erased
// 		// Retry nvs_flash_init
// 		ESP_ERROR_CHECK(nvs_flash_erase());
// 		err = nvs_flash_init();
// 	}
// 	ESP_ERROR_CHECK( err );

// 	ESP_LOGI(TAG, "Initializing SPIFFS");
// 	esp_err_t ret;
// 	ret = mountSPIFFS("/spiffs", "storage0", 10);
// 	if (ret != ESP_OK) return;
// 	listSPIFFS("/spiffs/");

// 	// Image file borrowed from here
// 	// https://www.flaticon.com/packs/social-media-343
// 	ret = mountSPIFFS("/icons", "storage1", 10);
// 	if (ret != ESP_OK) return;
// 	listSPIFFS("/icons/");

// 	ret = mountSPIFFS("/images", "storage2", 14);
// 	if (ret != ESP_OK) return;
// 	listSPIFFS("/images/");

// 	lcd_interface_cfg(&dev, INTERFACE);
// 	INIT_FUNCTION(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
// 	InitFontx(fx16G,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
// 	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
// 	InitFontx(fx32G,"/spiffs/ILGH32XB.FNT",""); // 16x32Dot Gothic
// 	InitFontx(fx16M,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
// 	InitFontx(fx24M,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
// 	InitFontx(fx32M,"/spiffs/ILMH32XB.FNT",""); // 16x32Dot Mincyo
// 	xTaskCreate(TFT, "TFT", 1024*6, NULL, 2, NULL);

// }
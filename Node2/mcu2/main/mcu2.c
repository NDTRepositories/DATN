#include <stdio.h>
#include "http.h"
#include "nvs_flash.h"
#include "UartComunication.h"
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lora.h"
#include "wifipeertopeer.h"
#include <freertos/event_groups.h>

#define MAX_COUNT 3
#define INITIAL_COUNT 0
#define BIT_0 (1 << 0)
#define BIT_1 (1 << 1)

char *get_received_message(void);
void example_wifi_init();
void example_espnow_init();    
void send_data_task_http(void *arg);
void uart_rx_task_http(void *arg);
void Timeout_task_http(void *arg);
void ESPNOWsendDatatoLora(void *pvParameters);
char *maintask = "TaskLoraNode";

SemaphoreHandle_t ShareDataformUarttoSIM=NULL; 
SemaphoreHandle_t xSemaphore = NULL;
SemaphoreHandle_t xSemaphoreforLora = NULL;
static EventGroupHandle_t xEventGroup;

void ManageDatafromUarttoSIM(void *arg)
{
	if(	xTaskCreatePinnedToCore(send_data_task_http, "send data uart task", 5120, NULL, 32, NULL, tskNO_AFFINITY) == pdPASS)
	{
		xTaskCreatePinnedToCore(uart_rx_task_http, "uart rx task", 4096, NULL, 1, NULL, tskNO_AFFINITY);
		xTaskCreatePinnedToCore(Timeout_task_http, "timeout task",3072, NULL, 3, NULL, tskNO_AFFINITY);		
	}
    vTaskDelete(NULL);
}


void TaskSIM(void *arg)
{
	if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) 
	{
	ESP_LOGI(pcTaskGetName(NULL), " Task SIM waiting for BIT_0");
	EventBits_t uxBits = xEventGroupWaitBits(
		xEventGroup, 
		BIT_0 | BIT_1,        
		pdTRUE,       
		pdFALSE,      
		portMAX_DELAY  
	);
	if((uxBits & BIT_0) == BIT_0)
	{
		ESP_LOGI(pcTaskGetName(NULL), " Task SIM received BIT_0");
		ManageDatafromUarttoSIM(NULL);
	}else if((uxBits & BIT_1) == BIT_1)
	{
		ESP_LOGI(pcTaskGetName(NULL), " Task SIM received BIT_1");
		vTaskDelete(NULL);	
	}
	}
}

#if CONFIG_PRIMARY

#define TIMEOUT 500
void task_primary(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	char recv_data[BUF_SIZE];
    static int count_timeout = 0;
	while(1) 
	{
        UBaseType_t stackSize = uxTaskGetStackHighWaterMark(NULL);
        size_t free_heap_size = esp_get_free_heap_size();
        ESP_LOGW(maintask," Remaining stack size of Primary Lora: %u, Free heap size: %zu bytes\n", stackSize, free_heap_size); 
        xSemaphoreTake(ShareDataformUarttoSIM, portMAX_DELAY);
        uart_receive_data(recv_data, BUF_SIZE - 1);
		char* message = get_received_message();
		int send_len =strlen(message);
		xSemaphoreGive(ShareDataformUarttoSIM);
		if(send_len > 0)
		{
		ESP_LOGI(maintask, "Lora Node 2: %s", message);
		xSemaphoreTake(xSemaphoreforLora, portMAX_DELAY);
		lora_send_packet((uint8_t*)message, send_len);
		xSemaphoreGive(xSemaphoreforLora);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent:[%.*s]", send_len, send_len, message);

		bool waiting1 = true;
		bool waiting2 = true;
		TickType_t startTick = xTaskGetTickCount();
		while(waiting1) 
		{
			lora_receive(); // put into receive mode
			if(lora_received()) 
			{

					int rxLen = lora_receive_packet(buf, sizeof(buf));
					if(rxLen < 10) 
					{
					xEventGroupSetBits(xEventGroup, BIT_1);
					TickType_t currentTick = xTaskGetTickCount();
					TickType_t diffTick = currentTick - startTick;

					ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
					ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" millisecond", diffTick * portTICK_PERIOD_MS);

					if(strstr((char *)buf, "NACK") != NULL)
					{
					xSemaphoreGive(xSemaphore);
					xSemaphoreGive(xSemaphore);	
					xSemaphoreGive(xSemaphore);	
					}
					waiting1 = false;
					}else 
					{
						vTaskDelay(pdMS_TO_TICKS(300));
						xSemaphoreTake(xSemaphoreforLora, portMAX_DELAY);
						lora_send_packet((uint8_t*)message, send_len);
						xSemaphoreGive(xSemaphoreforLora);
						ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent:[%.*s]", send_len, send_len, message);	
						while (waiting2)
						{
						lora_receive(); // put into receive mode
						if(lora_received()) 
						{
							xEventGroupSetBits(xEventGroup, BIT_1);
							int rxLen = lora_receive_packet(buf, sizeof(buf));
							TickType_t currentTick = xTaskGetTickCount();
							TickType_t diffTick = currentTick - startTick;

							ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
							ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" millisecond", diffTick * portTICK_PERIOD_MS);

							if(strstr((char *)buf, "NACK") != NULL)
							{
							xSemaphoreGive(xSemaphore);
							xSemaphoreGive(xSemaphore);	
							xSemaphoreGive(xSemaphore);	
							}
							waiting2 = false;
						}else
						{
						waiting2 = false;
						}
                       }
					   waiting1 = false;
			        }
			count_timeout = 0;
			ESP_LOGI(pcTaskGetName(NULL), "COUNT TIMEOUT=%d", count_timeout);
			}	   
			
			TickType_t currentTick = xTaskGetTickCount();
			TickType_t diffTick = currentTick - startTick;
			ESP_LOGD(pcTaskGetName(NULL), "diffTick=%"PRIu32, diffTick);

			if (diffTick > TIMEOUT) 
			{
				ESP_LOGW(pcTaskGetName(NULL), "Count Timeout");
                count_timeout++;
				waiting1 = false;
			}

			if(count_timeout > 21)
			{
				ESP_LOGI(pcTaskGetName(NULL), "COUNT TIMEOUT=%d", count_timeout);
				ESP_LOGW(pcTaskGetName(NULL), " Timeout,Start using SIM and ESP-NOW");
				ESP_LOGI(pcTaskGetName(NULL), "Task Lora setting BIT_0");
				xEventGroupSetBits(xEventGroup, BIT_0);
				xSemaphoreGive(xSemaphore);
				xSemaphoreGive(xSemaphore);	
				xSemaphoreGive(xSemaphore);	
				waiting1 = false;
			}

		vTaskDelay(1); // Avoid WatchDog alerts	
		}
		 // end waiting
	}
	vTaskDelay(pdMS_TO_TICKS(7500));
} // end while
 	
	vTaskDelete(NULL);
}
#endif // CONFIG_PRIMARY

#if CONFIG_SECONDARY
void task_secondary(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		lora_receive(); // put into receive mode
		if(lora_received()) {
			int rxLen = lora_receive_packet(buf, sizeof(buf));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
			for (int i=0;i<rxLen;i++) {
				if (isupper(buf[i])) {
					buf[i] = tolower(buf[i]);
				} else {
					buf[i] = toupper(buf[i]);
				}
			}
			vTaskDelay(1);
			lora_send_packet(buf, rxLen);
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", rxLen);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	}
}
#endif // CONFIG_SECONDARY

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
	uart_init(9, 10);
    ESP_ERROR_CHECK( ret );
	example_wifi_init();

	if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}
    ShareDataformUarttoSIM = xSemaphoreCreateMutex();
    if (ShareDataformUarttoSIM == NULL) {
        ESP_LOGE(maintask, "Create mutex fail");
        return;
	}

    xSemaphore = xSemaphoreCreateCounting(MAX_COUNT, INITIAL_COUNT);
    if (xSemaphore == NULL) {
        ESP_LOGE(maintask, "Failed to create semaphore");
        return;
    }
    xSemaphoreforLora = xSemaphoreCreateMutex();
	if (xSemaphoreforLora == NULL) {
		ESP_LOGE(maintask, "Create mutex fail");
		return;
	}
    xEventGroup = xEventGroupCreate();

    if (xEventGroup == NULL) {
        ESP_LOGE(maintask, "Failed to create event group");
        return;
    }

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
	xTaskCreatePinnedToCore(&task_primary, "PRIMARY", 5120, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(&TaskSIM, "TaskSIM", 4096, NULL, 5, NULL, tskNO_AFFINITY);
    example_espnow_init();
	xTaskCreatePinnedToCore(&ESPNOWsendDatatoLora, "ESPNOWsendDatatoLora", 3072, NULL, 4, NULL, tskNO_AFFINITY);
#endif
#if CONFIG_SECONDARY
	xTaskCreate(&task_secondary, "SECONDARY", 1024*8, NULL, 5, NULL);
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
// 		int send_len = sprintf((char *)buf,"HelloWorld!!Hello World!!Hello World!!Hello World!!Hello");

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
// 			vTaskDelay(1);
// 			lora_send_packet(buf, rxLen);
// 			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", rxLen);
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


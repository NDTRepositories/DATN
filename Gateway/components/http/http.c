#include <stdio.h>
#include "http.h"
#include "datamanager.h"
uint8_t ATC_Sent_TimeOut = 0;
char AT_BUFFER[AT_BUFFER_SZ] = "";
char AT_BUFFER1[AT_BUFFER_SZ] = "";
char AT_BUFFER2[AT_BUFFER_SZ] = "";
SIMCOM_ResponseEvent_t AT_RX_event;
char* get_received_message(void);
bool Flag_Wait_Exit = false ; // flag for wait response from sim or Timeout and exit loop
bool Flag_Device_Ready = false ; // Flag for SIM ready to use, False: device not connect, check by use AT\r\n command
ATCommand_t SIMCOM_ATCommand;

char *SIM_TAG = "UART SIM";
char *TAG_UART = "UART";
char *TAG_ATCommand= "AT COMMAND";


void SendATCommand()
{
	uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)SIMCOM_ATCommand.CMD, strlen(SIMCOM_ATCommand.CMD));
	ESP_LOGI(TAG_ATCommand,"Send:%s\n",SIMCOM_ATCommand.CMD);
	ESP_LOGI(TAG_ATCommand,"Packet:\n-ExpectResponseFromATC:%s\n-RetryCountATC:%d\n-TimeoutATC:%ld\n-CurrentTimeoutATC:%ld",SIMCOM_ATCommand.ExpectResponseFromATC
			,SIMCOM_ATCommand.RetryCountATC,SIMCOM_ATCommand.TimeoutATC,SIMCOM_ATCommand.CurrentTimeoutATC);
	Flag_Wait_Exit = false;
}
void ATC_SendATCommand(const char * Command, char *ExpectResponse, uint32_t timeout, uint8_t RetryCount, SIMCOM_SendATCallBack_t Callback){
	strcpy(SIMCOM_ATCommand.CMD, Command);
	SIMCOM_ATCommand.lenCMD = strlen(SIMCOM_ATCommand.CMD);
	strcpy(SIMCOM_ATCommand.ExpectResponseFromATC, ExpectResponse);
	SIMCOM_ATCommand.RetryCountATC = RetryCount;
	SIMCOM_ATCommand.SendATCallBack = Callback;
	SIMCOM_ATCommand.TimeoutATC = timeout;
	SIMCOM_ATCommand.CurrentTimeoutATC = 0;
	SendATCommand();
}


void RetrySendATC(){
	SendATCommand();
}


void ATResponse_callback(SIMCOM_ResponseEvent_t event, void *ResponseBuffer){
	AT_RX_event  = event;
	if(event == EVENT_OK){
		ESP_LOGI(TAG_ATCommand, "Device is ready to use\r\n");
		Flag_Wait_Exit = true;
		Flag_Device_Ready = true;
	}
	else if(event == EVENT_TIMEOUT){
		ESP_LOGE(TAG_ATCommand, "Timeout, Device is not ready\r\n");
		Flag_Wait_Exit = true;
	}
	else if(event == EVENT_ERROR){
		ESP_LOGE(TAG_ATCommand, "AT check Error \r\n");
		Flag_Wait_Exit = true;
	}
}


void  WaitandExitLoop(bool *Flag)
{
	while(1)
	{
		if(*Flag == true)
		{
			*Flag = false;
			break;
		}
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}


bool check_SIMA7670C(void)
{
	ATC_SendATCommand("AT\r\n", "OK", 2000, 4, ATResponse_callback);
	WaitandExitLoop(&Flag_Wait_Exit);
	if(AT_RX_event == EVENT_OK){
		return true;
	}else{
		return false;
	}
}



void Timeout_task_http(void *arg)
{

	
	UBaseType_t stackSize = uxTaskGetStackHighWaterMark(NULL);
	size_t free_heap_size = esp_get_free_heap_size();	
	ESP_LOGW(TAG_UART," Remaining stack size of time out : %u, Free heap size: %zu bytes\n", stackSize, free_heap_size);
	while (1)
	{
		
		if((SIMCOM_ATCommand.TimeoutATC > 0) && (SIMCOM_ATCommand.CurrentTimeoutATC < SIMCOM_ATCommand.TimeoutATC))
		{
			SIMCOM_ATCommand.CurrentTimeoutATC += TIMER_ATC_PERIOD;
			if(SIMCOM_ATCommand.CurrentTimeoutATC >= SIMCOM_ATCommand.TimeoutATC)
			{
				SIMCOM_ATCommand.CurrentTimeoutATC -= SIMCOM_ATCommand.TimeoutATC;
				if(SIMCOM_ATCommand.RetryCountATC > 0)
				{
					ESP_LOGI(SIM_TAG,"retry count %d",SIMCOM_ATCommand.RetryCountATC-1);
					SIMCOM_ATCommand.RetryCountATC--;
					RetrySendATC();
				}
				else
				{
					if(SIMCOM_ATCommand.SendATCallBack != NULL)
					{
						printf("Time out!\n"); 

						SIMCOM_ATCommand.TimeoutATC = 0;
						SIMCOM_ATCommand.SendATCallBack(EVENT_TIMEOUT, "@@@");
						ATC_Sent_TimeOut = 1;
					}
				}
			}
		}
		vTaskDelay(TIMER_ATC_PERIOD / portTICK_PERIOD_MS);
	}
 vTaskDelete(NULL);
}
bool isSignificantChange(float latitude_current, float longitude_current, float latitude_prev,float longitude_prev, float threshold) 
{
    return (fabs(latitude_current - latitude_prev) > threshold || fabs(longitude_current- longitude_prev) > threshold);

}

void SendGPS1toServer(float latitude, float longitude,float threshold)
{
		static float latitude_prev1 = 0;
		static float longitude_prev1 = 0;

		if (isSignificantChange(latitude, longitude, latitude_prev1, longitude_prev1, threshold))
		{
		latitude_prev1 = latitude;
		longitude_prev1 = longitude;
		sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/device/QDEDw9snWnmS03W7wYnlkLddeg/location?lat=%.6f&lon=%.6f\"\r\n",latitude,longitude);
		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);		
		WaitandExitLoop(&Flag_Wait_Exit);											
		ATC_SendATCommand("AT+HTTPACTION=0\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ESP_LOGW(TAG_UART, "Node 1 Send GPS to Server\n");


		}

}
void SendGPS2toServer(float latitude, float longitude,float threshold)
{
		static float latitude_prev2 = 0;
		static float longitude_prev2 = 0;

		if (isSignificantChange(latitude, longitude, latitude_prev2, longitude_prev2, threshold))
		{
		latitude_prev2 = latitude;
		longitude_prev2 = longitude;
		sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/device/QDEDw9snWnmS03W7wYSG6vi7mo/location?lat=%.6f&lon=%.6f\"\r\n",latitude,longitude);
		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);		
		WaitandExitLoop(&Flag_Wait_Exit);											
		ATC_SendATCommand("AT+HTTPACTION=0\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ESP_LOGW(TAG_UART, "Node 2 Send GPS to Server\n");

		}

}
void SendGPSGateWaytoServer(float latitude, float longitude,float threshold)
{
		static float latitude_prev = 0;
		static float longitude_prev = 0;

		if (isSignificantChange(latitude, longitude, latitude_prev, longitude_prev, threshold))
		{
		latitude_prev = latitude;
		longitude_prev = longitude;
		ATC_SendATCommand("AT+HTTPINIT\r\n", "OK", 2000, 2,ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);			
		sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/gateway/QDEDw9snWnmS03W7wY/location?lat=%.6f&lon=%.6f\"\r\n",latitude,longitude);
		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);		
		WaitandExitLoop(&Flag_Wait_Exit);											
		ATC_SendATCommand("AT+HTTPACTION=0\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPTERM\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);			
		ESP_LOGW(TAG_UART, "GATEWAY Send GPS to Server\n");
		}

}

void SendDataToServer(int id, float PH, float DO, float EC, float temperature, float latitude, float longitude,float threshold)
{
	

	if(check_SIMA7670C())
	{
		Flag_Device_Ready = true;
		ESP_LOGI(SIM_TAG,"Connected to SIMA7670C\r\n");
		ATC_SendATCommand("AT+HTTPINIT\r\n", "OK", 2000, 2,ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);	
		sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/data/sendData2?\"\r\n");
		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);
		sprintf(AT_BUFFER1 ,"AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
		ATC_SendATCommand(AT_BUFFER1, "OK", 10000, 3, ATResponse_callback);		
		WaitandExitLoop(&Flag_Wait_Exit);
		
        if(id == 1)
		{
		cJSON *root = cJSON_CreateObject();
		cJSON *devices = cJSON_CreateArray();

		cJSON_AddItemToObject(root, "gateway_API", cJSON_CreateString("QDEDw9snWnmS03W7wY"));
		cJSON_AddItemToObject(root, "devices", devices);

		cJSON *device1 = cJSON_CreateObject();
		cJSON *sensorData1 = cJSON_CreateObject();
		cJSON_AddItemToObject(device1, "device_API", cJSON_CreateString("QDEDw9snWnmS03W7wYnlkLddeg"));
		cJSON_AddItemToObject(device1, "sensorData", sensorData1);
		cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMgWG68o4b", cJSON_CreateNumber(DO));
		cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMQMeonYig", cJSON_CreateNumber(EC));
		cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMLSw1AYaMy", cJSON_CreateNumber(PH));
		cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMvQHcZFas", cJSON_CreateNumber(temperature));
		cJSON_AddItemToArray(devices, device1);

	    char *json_string = cJSON_Print(root);


		char content_length[16];
		sprintf(content_length, "%d", strlen(json_string));
		// Construct AT command string
		char atCommandStr[50];
		strcpy(atCommandStr, "AT+HTTPDATA=");
		strcat(atCommandStr, content_length);
		strcat(atCommandStr, ",1000\r\n");

		ATC_SendATCommand(atCommandStr, "OK", 2000, 0, NULL);

		vTaskDelay(1000/portTICK_PERIOD_MS);

		ATC_SendATCommand(json_string, "OK", 5000, 0, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		
		ATC_SendATCommand("AT+HTTPACTION=1\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);

		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);


		SendGPS1toServer(latitude,longitude,threshold);
        ESP_LOGW(TAG_UART,"NODE 1 Send data to server\n");

		cJSON_Delete(root);
		free(json_string);

		}

        if(id == 2)
		{
	    cJSON *root1 = cJSON_CreateObject();
	    cJSON *devices1 = cJSON_CreateArray();
	    cJSON_AddItemToObject(root1, "gateway_API", cJSON_CreateString("QDEDw9snWnmS03W7wY"));
	    cJSON_AddItemToObject(root1, "devices", devices1);	    
	    
	    cJSON *device2 = cJSON_CreateObject();
	    cJSON *sensorData2 = cJSON_CreateObject();
	    cJSON_AddItemToObject(device2, "device_API", cJSON_CreateString("QDEDw9snWnmS03W7wYSG6vi7mo"));
	    cJSON_AddItemToObject(device2, "sensorData", sensorData2);
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moRt5JKdBE", cJSON_CreateNumber(DO));--
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moZUniGrV7", cJSON_CreateNumber(EC));
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moMQEq8ztO", cJSON_CreateNumber(PH));
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moCtligoVx", cJSON_CreateNumber(temperature));
	    cJSON_AddItemToArray(devices1, device2);

	    
	    char *json_string1 = cJSON_Print(root1);	
	    
	        		       
		char content_length1[16];
		sprintf(content_length1, "%d", strlen(json_string1));
		// Construct AT command string
		char atCommandStr1[50];
		strcpy(atCommandStr1, "AT+HTTPDATA=");
		strcat(atCommandStr1, content_length1);
		strcat(atCommandStr1, ",1000\r\n");

		ATC_SendATCommand(atCommandStr1, "OK", 2000, 0, NULL);		

		vTaskDelay(1000/portTICK_PERIOD_MS);
				
		ATC_SendATCommand(json_string1, "OK", 5000, 0, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
										
		ATC_SendATCommand("AT+HTTPACTION=1\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);

		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);	


        SendGPS2toServer(latitude,longitude,threshold);

        ESP_LOGW(TAG_UART,"NODE 2 Send data to server\n");

		cJSON_Delete(root1);
		free(json_string1);

		}
		ATC_SendATCommand("AT+HTTPTERM\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);		

}
}

bool hasDataChanged(dataSensor_st currentData, dataSensor_st previousData) {
    return currentData.PH != previousData.PH ||
           currentData.DO != previousData.DO ||
           currentData.EC != previousData.EC ||
           currentData.temperature != previousData.temperature ||
           currentData.latitude != previousData.latitude ||
           currentData.longitude != previousData.longitude;
}


void send_data_task_http(void *arg)
{
	int count2 = 1;
	int count1 = 1;
	static dataSensor_st checkdataforsim_prev1 = {0,0,0,0,0,0,0};
    static dataSensor_st checkdataforsim_prev2 = {0,0,0,0,0,0,0};

	while (1)
	{
	UBaseType_t stackSize = uxTaskGetStackHighWaterMark(NULL);
	size_t free_heap_size = esp_get_free_heap_size();	

		if(dataforSim.id == 1)
		{

		if (hasDataChanged(dataforSim, checkdataforsim_prev1)) 
		{
		ESP_LOGI(pcTaskGetName(NULL),"Received Data to Device %d WITH VALAUE D0 =%.2f, EC =%.2f, PH = %.2f, TEMP = %.2f, LATI = %.6f, LONGTI = %.6f\n",dataforSim.id, dataforSim.DO, dataforSim.EC, dataforSim.PH, dataforSim.temperature, dataforSim.latitude, dataforSim.longitude);
		ESP_LOGW(TAG_UART," Remaining stack size of send data sim : %u, Free heap size: %zu bytes\n", stackSize, free_heap_size);	
		ESP_LOGI(TAG_UART, "Ready to send server");		

		xSemaphoreTake(xSemaphore, portMAX_DELAY);
		SendDataToServer(dataforSim.id, dataforSim.PH, dataforSim.DO, dataforSim.EC, dataforSim.temperature, dataforSim.latitude, dataforSim.longitude,0.000165);
		xSemaphoreGive(xSemaphore);
		checkdataforsim_prev1 = dataforSim;
		ESP_LOGW(TAG_UART,"Count Send Node 1 : %d\n", count1);
		count1++;	
		}
		}

		if(dataforSim.id == 2)
		{
		if (hasDataChanged(dataforSim, checkdataforsim_prev2)) 
		{
		ESP_LOGI(pcTaskGetName(NULL),"Received Data to Device %d WITH VALAUE D0 =%.2f, EC =%.2f, PH = %.2f, TEMP = %.2f, LATI = %.6f, LONGTI = %.6f\n",dataforSim.id, dataforSim.DO, dataforSim.EC, dataforSim.PH, dataforSim.temperature, dataforSim.latitude, dataforSim.longitude);
		ESP_LOGW(TAG_UART," Remaining stack size of send data sim : %u, Free heap size: %zu bytes\n", stackSize, free_heap_size);	
		ESP_LOGI(TAG_UART, "Ready to send server");		
		xSemaphoreTake(xSemaphore, portMAX_DELAY);
		SendDataToServer(dataforSim.id, dataforSim.PH, dataforSim.DO, dataforSim.EC, dataforSim.temperature, dataforSim.latitude, dataforSim.longitude,0.000165);
		xSemaphoreGive(xSemaphore);
		checkdataforsim_prev2 = dataforSim;
		ESP_LOGW(TAG_UART,"Count Send Node 2 : %d\n", count2);
		count2++;	
		}
		}
	vTaskDelay(50/ portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

void uart_rx_task_http(void *arg)
{
	uart_config_t uart_config = {
		.baud_rate = ECHO_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};
	int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
	intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif
	uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, BUF_SIZE, 0, NULL, intr_alloc_flags);
	uart_param_config(ECHO_UART_PORT_NUM, &uart_config);
	uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
	uint8_t data[BUF_SIZE];

	UBaseType_t stackSize = uxTaskGetStackHighWaterMark(NULL);
	size_t free_heap_size = esp_get_free_heap_size();	
	ESP_LOGW(TAG_UART," Remaining stack size of uart sim : %u, Free heap size: %zu bytes\n", stackSize, free_heap_size);	

	while (1)
	{
		int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
	
		if (len > 0)
		{
			data[len] = 0;
			ESP_LOGI(TAG_UART, "Rec: \r\n%s", data);
			if (SIMCOM_ATCommand.ExpectResponseFromATC[0] != 0 && strstr((const char *)data, SIMCOM_ATCommand.ExpectResponseFromATC))
			{
				SIMCOM_ATCommand.ExpectResponseFromATC[0] = 0;
				if (SIMCOM_ATCommand.SendATCallBack != NULL)
				{
					SIMCOM_ATCommand.TimeoutATC = 0;
					SIMCOM_ATCommand.SendATCallBack(EVENT_OK, data);
				}
			}
			if (strstr((const char *)data, "ERROR"))
			{

				if (SIMCOM_ATCommand.SendATCallBack != NULL)
				{
					SIMCOM_ATCommand.SendATCallBack(EVENT_ERROR, data);
				}
			}

		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}



void Timeout_task(void *arg)
{
	while (1)
	{
		if((SIMCOM_ATCommand.TimeoutATC > 0) && (SIMCOM_ATCommand.CurrentTimeoutATC < SIMCOM_ATCommand.TimeoutATC))
		{
			SIMCOM_ATCommand.CurrentTimeoutATC += TIMER_ATC_PERIOD;
			if(SIMCOM_ATCommand.CurrentTimeoutATC >= SIMCOM_ATCommand.TimeoutATC)
			{
				SIMCOM_ATCommand.CurrentTimeoutATC -= SIMCOM_ATCommand.TimeoutATC;
				if(SIMCOM_ATCommand.RetryCountATC > 0)
				{
					ESP_LOGI(SIM_TAG,"retry count %d",SIMCOM_ATCommand.RetryCountATC-1);
					SIMCOM_ATCommand.RetryCountATC--;
					RetrySendATC();
				}
				else
				{
					if(SIMCOM_ATCommand.SendATCallBack != NULL)
					{
						printf("Time out!\n"); 

						SIMCOM_ATCommand.TimeoutATC = 0;
						SIMCOM_ATCommand.SendATCallBack(EVENT_TIMEOUT, "@@@");
						ATC_Sent_TimeOut = 1;
					}
				}
			}
		}
		vTaskDelay(TIMER_ATC_PERIOD / portTICK_PERIOD_MS);
	}

}
void Connect_Server(void)
{
	if(check_SIMA7670C()){
		Flag_Device_Ready = true;
		ESP_LOGI(SIM_TAG,"Connected to SIMA7670C\r\n");
		ATC_SendATCommand("AT+HTTPINIT\r\n", "OK", 2000, 2,ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/data/sendData2?\"\r\n");
		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);
		sprintf(AT_BUFFER1 ,"AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n");
		ATC_SendATCommand(AT_BUFFER1, "OK", 10000, 3, ATResponse_callback);		
		WaitandExitLoop(&Flag_Wait_Exit);
		
    cJSON *root = cJSON_CreateObject();
    cJSON *devices = cJSON_CreateArray();

    cJSON_AddItemToObject(root, "gateway_API", cJSON_CreateString("QDEDw9snWnmS03W7wY"));
    cJSON_AddItemToObject(root, "devices", devices);

    cJSON *device1 = cJSON_CreateObject();
    cJSON *sensorData1 = cJSON_CreateObject();
    cJSON_AddItemToObject(device1, "device_API", cJSON_CreateString("QDEDw9snWnmS03W7wYnlkLddeg"));
    cJSON_AddItemToObject(device1, "sensorData", sensorData1);
    cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMgWG68o4b", cJSON_CreateNumber(1.12));
    cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMQMeonYig", cJSON_CreateNumber(2.12));
    cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMLSw1AYaMy", cJSON_CreateNumber(3.12));
    cJSON_AddItemToObject(sensorData1, "7W85OlSssnfaoLwgczSK4zb5EMvQHcZFas", cJSON_CreateNumber(4.12));
    cJSON_AddItemToArray(devices, device1);
    
	    cJSON *root1 = cJSON_CreateObject();
	    cJSON *devices1 = cJSON_CreateArray();
	    cJSON_AddItemToObject(root1, "gateway_API", cJSON_CreateString("QDEDw9snWnmS03W7wY"));
	    cJSON_AddItemToObject(root1, "devices", devices1);	    
	    
	    cJSON *device2 = cJSON_CreateObject();
	    cJSON *sensorData2 = cJSON_CreateObject();
	    cJSON_AddItemToObject(device2, "device_API", cJSON_CreateString("QDEDw9snWnmS03W7wYSG6vi7mo"));
	    cJSON_AddItemToObject(device2, "sensorData", sensorData2);
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moRt5JKdBE", cJSON_CreateNumber(11.12));
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moZUniGrV7", cJSON_CreateNumber(6.12));
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moMQEq8ztO", cJSON_CreateNumber(7.12));
	    cJSON_AddItemToObject(sensorData2, "QDEDw9snWnmS03W7wYSG6vi7moCtligoVx", cJSON_CreateNumber(8.12));
	    cJSON_AddItemToArray(devices1, device2);

	    char *json_string = cJSON_Print(root);
	    
	    char *json_string1 = cJSON_Print(root1);	
	    
	        		       
	        char content_length[16];
	        sprintf(content_length, "%d", strlen(json_string));
		  // Construct AT command string
		char atCommandStr[50];
		strcpy(atCommandStr, "AT+HTTPDATA=");
		strcat(atCommandStr, content_length);
		strcat(atCommandStr, ",1000\r\n");

		ATC_SendATCommand(atCommandStr, "OK", 2000, 0, NULL);
		

		vTaskDelay(1000/portTICK_PERIOD_MS);

		ATC_SendATCommand(json_string, "OK", 5000, 0, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		
		ATC_SendATCommand("AT+HTTPACTION=1\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
				
	        char content_length1[16];
	        sprintf(content_length1, "%d", strlen(json_string1));
		  // Construct AT command string
		char atCommandStr1[50];
		strcpy(atCommandStr1, "AT+HTTPDATA=");
		strcat(atCommandStr1, content_length1);
		strcat(atCommandStr1, ",1000\r\n");

		ATC_SendATCommand(atCommandStr1, "OK", 2000, 0, NULL);		
		//WaitandExitLoop(&Flag_Wait_Exit);
		vTaskDelay(1000/portTICK_PERIOD_MS);
				
		ATC_SendATCommand(json_string1, "OK", 5000, 0, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
										
		ATC_SendATCommand("AT+HTTPACTION=1\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPHEAD\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);			
						
		cJSON_Delete(root);
		free(json_string);                
}
}

void send_data_task(void *arg)
{
	int count = 1;
	while (1)
	{
		ESP_LOGI(TAG_UART, "Ready to send server");
	        Connect_Server();
		printf("count send:%d\n", count);
		count++;
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}

void uart_rx_task(void *arg)
{
	uart_config_t uart_config = {
		.baud_rate = ECHO_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_RTC,
		//.source_clk = UART_SCLK_APB,
	};
	int intr_alloc_flags = 0;
#if CONFIG_UART_ISR_IN_IRAM
	intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif
	uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags);
	uart_param_config(ECHO_UART_PORT_NUM, &uart_config);
	uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
	uint8_t data[BUF_SIZE];
	while (1)
	{
		int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
		if (len > 0)
		{
			data[len] = 0;
			ESP_LOGI(TAG_UART, "Rec: \r\n%s", data);
			if (SIMCOM_ATCommand.ExpectResponseFromATC[0] != 0 && strstr((const char *)data, SIMCOM_ATCommand.ExpectResponseFromATC))
			{
				SIMCOM_ATCommand.ExpectResponseFromATC[0] = 0;
				if (SIMCOM_ATCommand.SendATCallBack != NULL)
				{
					SIMCOM_ATCommand.TimeoutATC = 0;
					SIMCOM_ATCommand.SendATCallBack(EVENT_OK, data);
				}
			}
			if (strstr((const char *)data, "ERROR"))
			{

				if (SIMCOM_ATCommand.SendATCallBack != NULL)
				{
					SIMCOM_ATCommand.SendATCallBack(EVENT_ERROR, data);
				}
			}

		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
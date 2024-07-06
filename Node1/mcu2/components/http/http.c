#include <stdio.h>
#include "http.h"
#include "UartComunication.h"
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
dataSensor_st previousData = {0, 0, 0, 0, 0, 0, 0};
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
	ESP_LOGW(TAG_UART," Remaining stack size of time out sim : %u, Free heap size: %zu bytes\n", stackSize, free_heap_size);   	
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

bool hasDataChanged(dataSensor_st currentData, dataSensor_st previousData) 
{
    return currentData.PH != previousData.PH ||
           currentData.DO != previousData.DO ||
           currentData.EC != previousData.EC ||
           currentData.temperature != previousData.temperature|| 
		   currentData.latitude != previousData.latitude ||
		   currentData.longitude != previousData.longitude;
}

void SendDataToServer()
{
	dataSensor_st dataSensor;

		Flag_Device_Ready = true;
		char recv_data[BUF_SIZE];
		uart_receive_data(recv_data, BUF_SIZE - 1);
		char* message = get_received_message();
		// ESP_LOGW(SIM_TAG, "Data of SIM: %s", message);
        decodeHexToData(message, &dataSensor);

		if(hasDataChanged(dataSensor, previousData)) 
		{
			if(check_SIMA7670C())
			{			
			ESP_LOGI(SIM_TAG,"Connected to SIMA7670C\r\n");			
			ATC_SendATCommand("AT+HTTPINIT\r\n", "OK", 2000, 2,ATResponse_callback);
			WaitandExitLoop(&Flag_Wait_Exit);			
			sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/data/sendData1?gateway_API=QDEDw9snWnmS03W7wY&device_API=QDEDw9snWnmS03W7wYnlkLddeg&7W85OlSssnfaoLwgczSK4zb5EMLw1AYaMy=%.2f&7W85OlSssnfaoLwgczSK4zb5EMQMeonYig=%.2f&7W85OlSssnfaoLwgczSK4zb5EMgWG68o4b=%.2f&7W85OlSssnfaoLwgczSK4zb5EMvQHcZFas=%.2f\"\r\n",dataSensor.DO,dataSensor.EC, dataSensor.PH, dataSensor.temperature);
			ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);				

			WaitandExitLoop(&Flag_Wait_Exit);											
			ATC_SendATCommand("AT+HTTPACTION=0\r\n","OK" ,2000, 2, ATResponse_callback);
			WaitandExitLoop(&Flag_Wait_Exit);
			ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
			WaitandExitLoop(&Flag_Wait_Exit);

			SendGPStoServer(dataSensor.latitude, dataSensor.longitude, 0.000165);
			
			ATC_SendATCommand("AT+HTTPTERM\r\n","OK" ,2000, 2, ATResponse_callback);
			WaitandExitLoop(&Flag_Wait_Exit);	
	     	previousData = dataSensor;
		}	
    }
}
bool isSignificantChange(float latitude_current, float longitude_current, float latitude_prev,float longitude_prev, float threshold) 
{
    return (fabs(latitude_current - latitude_prev) > threshold || fabs(longitude_current- longitude_prev) > threshold);

}

void SendGPStoServer(float latitude, float longitude,float threshold)
{
		static float latitude_prev = 0;
		static float longitude_prev = 0;
		if (isSignificantChange(latitude, longitude, latitude_prev, longitude_prev, threshold))
		{
		latitude_prev = latitude;
		longitude_prev = longitude;		
		sprintf(AT_BUFFER ,"AT+HTTPPARA=\"URL\",\"http://sanslab.ddns.net:5000/api/device/QDEDw9snWnmS03W7wYnlkLddeg/location?lat=%.6f&lon=%.6f\"\r\n",latitude,longitude);
		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);		
		WaitandExitLoop(&Flag_Wait_Exit);											
		ATC_SendATCommand("AT+HTTPACTION=0\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+HTTPREAD=0,500\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
        ESP_LOGI(SIM_TAG,"Send GPS to server\n");
		}

}

void send_data_task_http(void *arg)
{
	while (1)
	{
		xSemaphoreTake(ShareDataformUarttoSIM, portMAX_DELAY);
	    SendDataToServer();
        xSemaphoreGive(ShareDataformUarttoSIM);
        vTaskDelay(50 / portTICK_PERIOD_MS);	
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


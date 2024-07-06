#include <stdio.h>
#include "tcp.h"

uint8_t ATC_Sent_TimeOut = 0;
char AT_BUFFER[AT_BUFFER_SZ] = "";
char AT_BUFFER1[AT_BUFFER_SZ] = "";
char AT_BUFFER2[AT_BUFFER_SZ] = "";
SIMCOM_ResponseEvent_t AT_RX_event;

bool Flag_Wait_Exit = false ; // flag for wait response from sim or Timeout and exit loop
bool Flag_Device_Ready = false ; // Flag for SIM ready to use, False: device not connect, check by use AT\r\n command
ATCommand_t SIMCOM_ATCommand;

char *SIM_TAG = "UART SIM";
char *TAG_UART = "UART";
char *TAG_ATCommand= "AT COMMAND";

Server server_sanslab ={
		.Server = SERVER,
		.Port = PORT,
};

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


void SendSMSMessage(char* phoneNumber, char* message){
    char atCommand[256];

    // Set SMS format to text mode
    ATC_SendATCommand("AT+CMGF=1\r\n", "OK", 2000, 4, ATResponse_callback);
    WaitandExitLoop(&Flag_Wait_Exit);

    // Set the phone number
    sprintf(atCommand, "AT+CMGS=\"%s\"\r\n", phoneNumber);
    ATC_SendATCommand(atCommand, ">", 2000, 4, ATResponse_callback);
    WaitandExitLoop(&Flag_Wait_Exit);

    // Set the message
    sprintf(atCommand, "%s\x1A", message); // End of message character (ASCII 26)
    ATC_SendATCommand(atCommand, "OK", 2000, 4, ATResponse_callback);
    WaitandExitLoop(&Flag_Wait_Exit);
}

void Timeout_task_tcp(void *arg)
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
		ATC_SendATCommand("AT+NETOPEN\r\n", "OK", 2000, 2,ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+IPADDR\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+CIPRXGET=1\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);

		sprintf(AT_BUFFER ,"AT+CIPOPEN=0,\"TCP\",\"%s\",\"%s\"\r\n",server_sanslab.Server,server_sanslab.Port);

		ATC_SendATCommand(AT_BUFFER, "OK", 10000, 3, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
	       
		sprintf(AT_BUFFER1,"GET /api/data/sendData1?gateway_API=QDEDw9snWnmS03W7wY&device_API=QDEDw9snWnmS03W7wYSG6vi7mo&QDEDw9snWnmS03W7wYSG6vi7moRt5JKdBE=%.2f&QDEDw9snWnmS03W7wYSG6vi7moZUniGrV7=%.2f&QDEDw9snWnmS03W7wYSG6vi7moMQEq8ztO=%.2f&QDEDw9snWnmS03W7wYSG6vi7moCtligoVx=%.2f HTTP/1.1\r\nHost: sanslab.viewdns.net:5000\r\nContent-Type: application/json\r\n\n",25.12,30.12,40.12,50.12);

		  // Determine data length
		int dataLength = strlen(AT_BUFFER1); // Assuming dataToSend is a string

		  // Convert data length to string
		char dataLengthStr[10]; // Assuming data length is less than 1000 bytes
		sprintf(dataLengthStr, "%d", dataLength);

		  // Construct AT command string
		char atCommandStr[50];
		strcpy(atCommandStr, "AT+CIPSEND=0,");
		strcat(atCommandStr, dataLengthStr);
		strcat(atCommandStr, "\r\n");
			
		ATC_SendATCommand(atCommandStr, "", 2000, 0, NULL);
		//WaitandExitLoop(&Flag_Wait_Exit);
		vTaskDelay(1000/portTICK_PERIOD_MS);

		ATC_SendATCommand(AT_BUFFER1, "OK", 5000, 0, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);
		ATC_SendATCommand("AT+NETCLOSE\r\n","OK" ,2000, 2, ATResponse_callback);
		WaitandExitLoop(&Flag_Wait_Exit);				

}
}
void send_data_task_tcp(void *arg)
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

void uart_rx_task_tcp(void *arg)
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
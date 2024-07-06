/*
 * mypH.c
 *
 *  Created on: Apr 28, 2023
 *      Author: duyph
 */
#include "../Lib/mypH.h"

#include "../Lib/nvs_interface.h"
#include <stdbool.h>


#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "../Lib/ADS1115.h"
#include "../Lib/i2cdev.h"
#define I2C_PORT 0
#define CONFIG_EXAMPLE_DEV_COUNT 1
#define GAIN ADS111X_GAIN_4V096

static const uint8_t addr[CONFIG_EXAMPLE_DEV_COUNT] = {
    ADS111X_ADDR_GND,
    ADS111X_ADDR_VCC
};

// Descriptors
static i2c_dev_t devices[CONFIG_EXAMPLE_DEV_COUNT];

// Gain value
static float gain_val;


// Main task



uint32_t _pH_4_voltage = 0;
uint32_t  _pH_6_86_voltage = 0;
uint32_t _pH_9_voltage  = 0;

bool flag_is_get_param = false;

bool param_is_null = true ;

double a_6_86_4 = 1.0;
double b_6_86_4 = 0.0;

double a_9_6_86 = 1.0 ;
double b_9_6_86 = 0.0;

void init_param_pH(nvs_handle_t nvsHandle){
	esp_err_t err;
	err = nvs_open("storage", NVS_READWRITE,&nvsHandle);
	if (err != ESP_OK) {
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	} else {
		printf("Done\n");

		// Read
		printf("Reading _pH_4_voltage from NVS ... ");
		err = read_nvs(nvsHandle,"storage",PH_4_VAL_ADDR, (uint32_t *)&_pH_4_voltage);

		switch (err) {
		case ESP_OK:
			printf("Done\n");
			printf("_pH_4_voltage = %ld\n", _pH_4_voltage);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			break;
		default :
			printf("Error (%s) reading!\n", esp_err_to_name(err));
		}
		err = read_nvs(nvsHandle, "storage",PH_6_86_VAL_ADDR, (uint32_t *)&_pH_6_86_voltage);
		switch (err) {
		case ESP_OK:
			printf("Done\n");
			printf("_pH_6_86_voltage = %ld\n", _pH_6_86_voltage);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			break;
		default :
			printf("Error (%s) reading!\n", esp_err_to_name(err));
		}
		err = read_nvs(nvsHandle,"storage", PH_9_VAL_ADDR,(uint32_t *) &_pH_9_voltage);
		switch (err) {
		case ESP_OK:
			printf("Done\n");
			printf("_pH_9_voltage = %ld\n", _pH_9_voltage);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			flag_is_get_param = false;
			break;
		default :
			printf("Error (%s) reading!\n", esp_err_to_name(err));
			flag_is_get_param = false;
		}
        printf("ok");
		flag_is_get_param = true;
		pH_function_9_6_86();
		pH_function_4_6_86();
	}
}

void pH_function_9_6_86(){
	a_9_6_86 = (9.0 - 6.86) / ((float)_pH_9_voltage/1000000.0 - (float)_pH_6_86_voltage/1000000.0);
	b_9_6_86 = 6.86 - (a_9_6_86 *(float)_pH_6_86_voltage/1000000.0);
	printf("a_9_6_86: %f \t b_9_6_86: %f \n",(float)a_9_6_86,(float)b_9_6_86);
}
void pH_function_4_6_86(){
	a_6_86_4 = (6.86 - 4.0) / ((float)_pH_6_86_voltage/1000000.0 - (float)_pH_4_voltage/1000000.0);
	b_6_86_4 = 4 - (a_6_86_4 *(float)_pH_4_voltage/1000000.0);
	printf("a_6_86_4: %f \t b_6_86_4: %f\n ",(float)a_6_86_4,(float)b_6_86_4);
}
void pH_Calib( float ADC_VREF, float ADC_resolution
		, nvs_handle_t nvsHandle
		, const char * space_name
		, pH_calib_t pH_calib_t){
	esp_err_t err;
	char * key= "";
	uint32_t avgValue;
	switch (pH_calib_t){
	case pH_CALIB_4:
		ESP_LOGI(TAG_pH_Calib,"Start calib pH 4");
		key = PH_4_VAL_ADDR;
		break;
	case pH_CALIB_6_86:
		ESP_LOGI(TAG_pH_Calib,"Start calib pH 6.86");
		key = PH_6_86_VAL_ADDR;
		break;
	case pH_CALIB_9:
		ESP_LOGI(TAG_pH_Calib,"Start calib pH 9");
		key = PH_9_VAL_ADDR;
		break;
	}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	memset(devices, 0, sizeof(devices));
    gain_val = ads111x_gain_values[GAIN];
    // Setup ICs
    for (size_t i = 0; i < CONFIG_EXAMPLE_DEV_COUNT; i++)
    {
        ESP_ERROR_CHECK(ads111x_init_desc(&devices[i], 0x48, I2C_PORT, 16, 15));

        ESP_ERROR_CHECK(ads111x_set_mode(&devices[i], ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
        ESP_ERROR_CHECK(ads111x_set_data_rate(&devices[i], ADS111X_DATA_RATE_32)); // 32 samples per second
        ESP_ERROR_CHECK(ads111x_set_input_mux(&devices[i], ADS111X_MUX_0_GND));    // positive = AIN0, negative = GND
        ESP_ERROR_CHECK(ads111x_set_gain(&devices[i], GAIN));
    }
	(void)addr; 
    uint8_t t ;  
	int32_t raw = 0;
	int buf [4];
	
    for (t=0;t<4;t++){
    // Read result
    if (ads111x_get_value(&devices[0], &raw) == ESP_OK)
    {
        float voltage = gain_val / ADS111X_MAX_VALUE * raw;
        printf("[%u] Raw ADC value: %ld, voltage: %.04f volts\n", 0, raw, voltage);
		// if(raw == 0){
		// ads111x_get_value(&devices[0], &raw);}
		buf[t] = raw;
		vTaskDelay(pdMS_TO_TICKS(200));
       // avg_adc = raw;
    }
    else
	{
        //printf("[%u] Cannot read ADC value\n", 0);
	}
    }

    for (int i = 0; i < 4; i++) {
       for (int j = i + 1; j < 4; j++) {
         if (buf[i] > buf[j]) {
            int temp = buf[i];
            buf[i] = buf[j];
            buf[j] = temp;
         }
        }
    }
    avgValue =0;
	for(int i = 1; i < 3 ; i ++){
		avgValue+= buf[i];
	}
	avgValue  /= 2;
	printf("ADC pH: %ld\n",avgValue);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    
	ESP_LOGI(TAG_pH,"avgVal ADC: %d",(int)avgValue);
	float voltage = convert_ADC_voltage(avgValue, ADC_resolution, ADC_VREF);
	avgValue = (uint32_t)voltage * 1000;
	ESP_LOGI(TAG_pH,"Saving param to NVS: %d",(int)avgValue);
	printf("key:%s\n",key);
	err = write_nvs_func(nvsHandle, space_name, key, (uint32_t)avgValue);
	ESP_LOGI(TAG_pH_Calib,"%s",esp_err_to_name(err));
	ESP_LOGI(TAG_pH_Calib,"%s",(err!= ESP_OK)?"Error in save to nvs!":"Calib success!");
	read_nvs(nvsHandle, space_name, key, &_pH_4_voltage);
}

float get_pH_value(uint32_t ADC_value,nvs_handle_t nvsHandle, uint32_t resolution, float V_ref){
	if(flag_is_get_param == false){
		ESP_LOGI(TAG_pH,"Reading param in EEPROM....");
		init_param_pH(nvsHandle);
		if(_pH_4_voltage == 0 || _pH_6_86_voltage == 0 || _pH_9_voltage == 0){
			ESP_LOGI(TAG_pH,"No param in EEPROM!");
			_pH_4_voltage = 0;
			_pH_6_86_voltage = 0;
			_pH_9_voltage = 0;
			return convert_ADC_voltage(ADC_value, resolution, V_ref);
		}

	}

	float voltage = convert_ADC_voltage(ADC_value, resolution, V_ref);
	//printf("voltage ph adc: %f\n",voltage);
	return (voltage/1000.0 <(float)_pH_6_86_voltage/1000000.0?(a_6_86_4*voltage/1000.0 + b_6_86_4)
			:(a_9_6_86*voltage/1000.0 +b_9_6_86));



}
float convert_ADC_voltage(uint32_t ADC, uint32_t resolution, float V_ref){
	return (float)ADC/(float)resolution * V_ref;
}

float get_pH(nvs_handle_t nvsHandle, float ADC_VREF, float ADC_resolution){
	
	uint32_t avg_adc = 0;
	float pH_val;


    memset(devices, 0, sizeof(devices));
    gain_val = ads111x_gain_values[GAIN];
    // Setup ICs
   // for (size_t i = 0; i < CONFIG_EXAMPLE_DEV_COUNT; i++)
   // {
	ESP_ERROR_CHECK(ads111x_init_desc(&devices[0], 0x48, I2C_PORT, 16, 15));

	ESP_ERROR_CHECK(ads111x_set_mode(&devices[0], ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
	ESP_ERROR_CHECK(ads111x_set_data_rate(&devices[0], ADS111X_DATA_RATE_32)); // 32 samples per second
	ESP_ERROR_CHECK(ads111x_set_input_mux(&devices[0], ADS111X_MUX_0_GND));    // positive = AIN0, negative = GND
	ESP_ERROR_CHECK(ads111x_set_gain(&devices[0], GAIN));
    //}
	(void)addr; 
    uint8_t t ;  
	int32_t raw = 0;
	int buf [3];
    for (t=0;t<3;t++){
    // Read result
   // if (ads111x_get_value(&devices[0], &raw) == ESP_OK)
   // {
		ads111x_get_value(&devices[0], &raw);
        float voltage = gain_val / ADS111X_MAX_VALUE * raw;
        printf("[%u] Raw ADC value: %ld, voltage: %.04f volts\n", 0, raw, voltage);
		// if(raw == 0){
		// ads111x_get_value(&devices[0], &raw);}
		buf[t] = raw;
		  vTaskDelay(500/portTICK_PERIOD_MS);
       // avg_adc = raw;
    // }
    // else
	// {
    //     //printf("[%u] Cannot read ADC value\n", 0);
	// }
    }

    for (int i = 0; i < 3; i++) {
       for (int j = i + 1; j < 3; j++) {
         if (buf[i] > buf[j]) {
            int temp = buf[i];
            buf[i] = buf[j];
            buf[j] = temp;
         }
        }
    }

	//for(int i = 1; i < 3 ; i ++){
		avg_adc = buf[1];
	// }
	// avg_adc  /= 2;
    printf("----------          ADC %ld       ---------\n",avg_adc);
   // printf("ADC pH: %ld\n",avg_adc);
	pH_val = get_pH_value(avg_adc, nvsHandle,ADC_resolution, ADC_VREF);
	return pH_val ;
}

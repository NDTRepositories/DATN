#include <stdio.h>
#include "ecsensor.h"
#include "nvs_interface.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "ads1115.h"
#include "i2cdev.h"
#define I2C_PORT 0
#define GAIN ADS111X_GAIN_4V096

static const uint8_t addr = ADS111X_ADDR_GND;

// Descriptors
static i2c_dev_t devices;

// Gain value
static float gain_val;

#define RES2 (7500.0/0.66)
#define ECREF 20.0

uint32_t  EC_kvalueLow;
uint32_t  EC_kvalueHigh;

void EC_init_param(nvs_handle_t nvs_handle){
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
	if (err != ESP_OK) {
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	} else {
		printf("Done\n");

		// Read
		printf("Reading EC_kvalue from NVS ... ");
		err = read_nvs(nvs_handle,"storage", KVALUEADDR_LOW, (uint32_t *)&EC_kvalueLow);

		switch (err) {
		case ESP_OK:
			printf("Done\n");

			printf("KVALUEADDR_LOW = %ld\n", EC_kvalueLow);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			EC_kvalueLow = 1000;
			write_nvs_func(nvs_handle, "storage", KVALUEADDR_LOW, (uint32_t)EC_kvalueLow);
			EC_kvalueLow = 1;
			break;
		default :
			printf("Error (%s) reading!\n", esp_err_to_name(err));
		}
		err = read_nvs(nvs_handle,"storage", KVALUEADDR_HIGH, (uint32_t *)&EC_kvalueHigh);
		switch (err) {
		case ESP_OK:
			printf("Done\n");

			printf("EC_kvalueHigh = %ld\n", EC_kvalueHigh);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			EC_kvalueHigh = 1000;
			write_nvs_func(nvs_handle, "storage", KVALUEADDR_HIGH, (uint32_t)EC_kvalueHigh);
			EC_kvalueHigh = 1;
			printf("The value is not initialized yet!\n");
			break;
		default :
			printf("Error (%s) reading!\n", esp_err_to_name(err));
		}
	}
}
static float convert_ADC_voltage(uint32_t avgValue, float ADC_resolution,float  ADC_Vref){
	return (float)avgValue*ADC_Vref/ADC_resolution;
}
void EC_calibrate(nvs_handle_t nvs_handle
		,float ADC_Vref
		, float ADC_resolution
		, const char *space_name
		, float EC_resolution
		, EC_calib_t EC_calib_type
		, float temperature){

	esp_err_t err;
	char *key="";
	float EC_solution = 0.0;
	uint32_t avgValue = 0;
	switch(EC_calib_type){
	case EC_calib_low_1413:
		ESP_LOGI(EC_TAG,"Start calib EC_calib_low_1413");
		EC_solution = 1.413;
		key = KVALUEADDR_LOW;
		break;
	case EC_calib_high_2_76:
		ESP_LOGI(EC_TAG,"Start calib EC_calib_high_2_76");
		EC_solution = 2.76;
		key = KVALUEADDR_HIGH;
		break;
	case EC_calib_hight_12_88:
		ESP_LOGI(EC_TAG,"Start calib EC_calib_hight_12_88");
		EC_solution = 12.88;
		key = KVALUEADDR_HIGH;
		break;
	}

    gain_val = ads111x_gain_values[GAIN];

	ESP_ERROR_CHECK(ads111x_init_desc(&devices, addr, I2C_PORT, CONFIG_ADS1115_I2C_MASTER_SDA, CONFIG_ADS1115_I2C_MASTER_SCL));

	ESP_ERROR_CHECK(ads111x_set_mode(&devices, ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
	ESP_ERROR_CHECK(ads111x_set_data_rate(&devices, ADS111X_DATA_RATE_32)); // 32 samples per second
	ESP_ERROR_CHECK(ads111x_set_input_mux(&devices, ADS111X_MUX_2_GND));    // positive = AIN0, negative = GND
	ESP_ERROR_CHECK(ads111x_set_gain(&devices, GAIN));

	int32_t raw = 0;
	int buf [4];
	
    for (uint8_t t = 0; t < 4 ;t++){
    // Read result
    if (ads111x_get_value(&devices,(int16_t*) &raw) == ESP_OK)
    {
        float voltage = gain_val / ADS111X_MAX_VALUE * raw;
        printf("[%u] Raw ADC value: %ld, voltage: %.04f volts\n", 0, raw, voltage);
		buf[t] = raw;
		vTaskDelay(pdMS_TO_TICKS(200));
    }
    else
	{
        printf("[%u] Cannot read ADC value\n", 0);
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
	for(int i = 1; i < 3 ; i ++){
		avgValue+= buf[i];
	}
	avgValue  /= 2;
	printf("ADC EC: %ld\n",avgValue);


	avgValue = 2138;
	float compECsolution = EC_solution* (1.0 + 0.0185 * (temperature - 25.0));
	float voltage = convert_ADC_voltage(avgValue,ADC_resolution,ADC_Vref);
	float KValueTemp = RES2 * ECREF * compECsolution / 1000.0 / voltage/10.0;
	KValueTemp *= 1000.0;
	err = write_nvs_func(nvs_handle, space_name, key, (uint32_t)KValueTemp);

	ESP_LOGI(EC_TAG,"%s",(err!= ESP_OK)?"Error in save to nvs!":"Calib success!");
	read_nvs(nvs_handle, space_name, key,(uint32_t*) &KValueTemp);
}

/*
 * Function get EC value
 * @param v_ref: 3300
 * return float
 * */
float EC_get_value(float ADC_resolution, float v_ref, float temperature){

	uint32_t avg_adc = 0;

    gain_val = ads111x_gain_values[GAIN];
    // Setup ICs
    
	ESP_ERROR_CHECK(ads111x_init_desc(&devices, addr, I2C_PORT, CONFIG_ADS1115_I2C_MASTER_SDA, CONFIG_ADS1115_I2C_MASTER_SCL));

	ESP_ERROR_CHECK(ads111x_set_mode(&devices, ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
	ESP_ERROR_CHECK(ads111x_set_data_rate(&devices, ADS111X_DATA_RATE_32)); // 32 samples per second
	ESP_ERROR_CHECK(ads111x_set_input_mux(&devices, ADS111X_MUX_2_GND));    // positive = AIN0, negative = GND
	ESP_ERROR_CHECK(ads111x_set_gain(&devices, GAIN));
    
	int32_t raw = 0;
	int buf [4];
    for (uint8_t t=0;t<4;t++)
	{
    // Read result
    if (ads111x_get_value(&devices,(int16_t*) &raw) == ESP_OK)
    {
        float voltage = gain_val / ADS111X_MAX_VALUE * raw;
        printf("[%u] Raw ADC value: %ld, voltage: %.04f volts\n", 0, raw, voltage);
		buf[t] = raw;
		vTaskDelay(pdMS_TO_TICKS(200));
    }
    else
	{
        printf("[%u] Cannot read ADC value\n", 0);
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

	for(int i = 1; i < 3 ; i ++)
	{
		avg_adc += buf[i];
	}
	avg_adc  /= 2;

   printf("ADC pH: %ld\n",avg_adc);


	float voltage = convert_ADC_voltage(avg_adc, ADC_resolution, v_ref);

	float value = (((voltage/RES2)/ECREF)*10.0);
	value = value * (float)EC_kvalueHigh;
	value = value/(1.0 + 0.0185*(temperature - 25.0));
	return value;
}
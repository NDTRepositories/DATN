#include <stdio.h>
#include "dosensor.h"
#include "nvs_interface.h"
#include <stdbool.h>
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
uint32_t DO_V0 = 0, DO_V100 = 0;
 float DO_a = 1, DO_b = 0;
  uint16_t DO_Table[41] = {
         14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
         11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
         9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
         7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};


void init_param_DO(nvs_handle_t pnvs_handle){
	esp_err_t err;
	err = nvs_open("storage", NVS_READWRITE, &pnvs_handle);
	if (err != ESP_OK) {
		ESP_LOGI(TAG_DO_calib,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	} else {
		ESP_LOGI(TAG_DO_calib,"Done\n");

		// Read
		ESP_LOGI(TAG_DO_calib,"Reading DO_0_VAL from NVS ... ");
		err = read_nvs(pnvs_handle,"storage", DO_0_VAL_ADDR, (uint32_t *)&DO_V0);

		switch (err) {
		case ESP_OK:
			ESP_LOGI(TAG_DO_calib,"Done\n");
			//DO_V0 /= 1000000.0;
			ESP_LOGI(TAG_DO_calib,"DO_0_VAL = %ld\n", DO_V0);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(TAG_DO_calib,"The value is not initialized yet!\n");
			break;
		default :
			ESP_LOGI(TAG_DO_calib,"Error (%s) reading!\n", esp_err_to_name(err));
		}
		err = read_nvs(pnvs_handle,"storage", DO_100_VAL_ADDR, (uint32_t *)&DO_V100);
		switch (err) {
		case ESP_OK:
			ESP_LOGI(TAG_DO_calib,"Done\n");
			//DO_V100 /= 1000000.0;
			ESP_LOGI(TAG_DO_calib,"DO_100_VAL = %ld\n", DO_V100);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(TAG_DO_calib,"The value is not initialized yet!\n");
			break;
		default :
			ESP_LOGI(TAG_DO_calib,"Error (%s) reading!\n", esp_err_to_name(err));
		}

	}
	DO_function();
}
float convert_ADC_voltage(uint32_t avgValue, float ADC_resolution,float  ADC_Vref){
	return (float)avgValue*ADC_Vref/ADC_resolution;
}
void DO_Calib(float ADC_Vref
		, float ADC_resolution
		, nvs_handle_t nvs_handle
		, const char * space_name
		, DO_calib_t do_calib){
	esp_err_t err;
	char * key= "";
	uint32_t avgValue =0;
	switch (do_calib){
	case do_calib_0:
		ESP_LOGI(TAG_DO_calib,"Start calib do 0");
		key = DO_0_VAL_ADDR;
		break;
	case do_calib_100:
		ESP_LOGI(TAG_DO_calib,"Start calib do 100");
		key = DO_100_VAL_ADDR;
		break;
	default:
		key = " ";
	}

    gain_val = ads111x_gain_values[GAIN];
    // Setup ICs

	ESP_ERROR_CHECK(ads111x_init_desc(&devices, addr, I2C_PORT, CONFIG_ADS1115_I2C_MASTER_SDA, CONFIG_ADS1115_I2C_MASTER_SCL));

	ESP_ERROR_CHECK(ads111x_set_mode(&devices, ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
	ESP_ERROR_CHECK(ads111x_set_data_rate(&devices, ADS111X_DATA_RATE_32)); // 32 samples per second
	ESP_ERROR_CHECK(ads111x_set_input_mux(&devices, ADS111X_MUX_1_GND));    // positive = AIN0, negative = GND
	ESP_ERROR_CHECK(ads111x_set_gain(&devices, GAIN));


    uint8_t t ;  
	int32_t raw = 0;
	int buf [4];
	
    for (t=0;t<4;t++){
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
	printf("ADC DO: %ld\n",avgValue);   
	avgValue = 40;
	float voltage = convert_ADC_voltage(avgValue, ADC_resolution, ADC_Vref);
	avgValue = (uint32_t)voltage * 1000;
	err = write_nvs_func(nvs_handle, space_name, key, (uint32_t)avgValue);

	ESP_LOGI(TAG_DO_calib,"%s",(err!= ESP_OK)?"Error in save to nvs!":"Calib success!");
	read_nvs(nvs_handle, space_name, key, &avgValue);
}


float get_standard_do(float temperature){
	return (DO_Table[(int)temperature] * temperature) / (int)temperature;
}
float convert_vol2mgL(float ADC_voltage,float temperature){
	float percentDO = DO_a * ADC_voltage + DO_b;
	return (percentDO * get_standard_do(temperature));
}
void DO_function(){
	ESP_LOGI(TAG_DO_calib,"DO 0V  = %ld\n",DO_V0);
	ESP_LOGI(TAG_DO_calib,"DO 100 = %ld\n",DO_V100);
	DO_a = 1 / ((float)DO_V100/1000000.0 - (float)DO_V0/1000000.0);
	DO_b = - (float)DO_V0/((float)DO_V100 - (float)DO_V0);
	ESP_LOGI(TAG_DO_calib,"DO a = %f\n",DO_a);
	ESP_LOGI(TAG_DO_calib,"DO b = %f\n",DO_b);
}
float get_DO(nvs_handle_t nvsHandle, float ADC_VREF, float ADC_resolution){

	uint32_t avg_adc = 0;


    gain_val = ads111x_gain_values[GAIN];
    // Setup ICs

	ESP_ERROR_CHECK(ads111x_init_desc(&devices, addr, I2C_PORT, CONFIG_ADS1115_I2C_MASTER_SDA, CONFIG_ADS1115_I2C_MASTER_SCL));

	ESP_ERROR_CHECK(ads111x_set_mode(&devices, ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
	ESP_ERROR_CHECK(ads111x_set_data_rate(&devices, ADS111X_DATA_RATE_32)); // 32 samples per second
	ESP_ERROR_CHECK(ads111x_set_input_mux(&devices, ADS111X_MUX_1_GND));    // positive = AIN0, negative = GND
	ESP_ERROR_CHECK(ads111x_set_gain(&devices, GAIN));

    uint8_t t ;  
	int32_t raw = 0;
	int buf [4];
    for (t=0;t<4;t++){
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
		avg_adc += buf[i];
	}
	avg_adc  /= 2;


			float adc_voltage = (float)avg_adc/(ADC_resolution + 1)*(ADC_VREF/1000.0);
			float DO_val = convert_vol2mgL(adc_voltage, 25.0);
			 DO_val = DO_val/1000;
			
			printf("DO ADC %ld\n",avg_adc);
			return (float)DO_val;
}

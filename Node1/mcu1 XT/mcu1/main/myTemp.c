#include "../Lib/myTemp.h"
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


float get_Temp(float res, float beta, float t0){
   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t avgValue;
	memset(devices, 0, sizeof(devices));
    gain_val = ads111x_gain_values[GAIN];
    // Setup ICs
    // for (size_t i = 0; i < CONFIG_EXAMPLE_DEV_COUNT; i++)
    // {
        ESP_ERROR_CHECK(ads111x_init_desc(&devices[0], 0x48, I2C_PORT, 16, 15));

        ESP_ERROR_CHECK(ads111x_set_mode(&devices[0], ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
        ESP_ERROR_CHECK(ads111x_set_data_rate(&devices[0], ADS111X_DATA_RATE_32)); // 32 samples per second
        ESP_ERROR_CHECK(ads111x_set_input_mux(&devices[0], ADS111X_MUX_3_GND));    // positive = AIN0, negative = GND
        ESP_ERROR_CHECK(ads111x_set_gain(&devices[0], GAIN));
   // }
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
    avgValue =0;
	//for(int i = 1; i < 3 ; i ++){
	avgValue = buf[1];
	//}
	//avgValue  /= 2;
    printf("----------          ADC %ld       ---------\n",avgValue);
    //printf("ADC : %ld\n",avgValue);
    float voltage = (float)avgValue/26400.0*3.3;
    float ntc_res =  (3.3 - voltage)/voltage *res   ;
    float Temp = 1.0 / (1.0 / t0 + log(ntc_res / res)/beta) - 273.0 ;
    return Temp;
	
}
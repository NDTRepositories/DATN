#include <stdio.h>
#include "temperature.h"
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


float get_Temp(float res, float beta, float t0)
{
    gain_val = ads111x_gain_values[GAIN];
    ESP_ERROR_CHECK(ads111x_init_desc(&devices, addr, I2C_PORT, CONFIG_ADS1115_I2C_MASTER_SDA, CONFIG_ADS1115_I2C_MASTER_SCL));
    ESP_ERROR_CHECK(ads111x_set_mode(&devices, ADS111X_MODE_CONTINUOUS));    // Continuous conversion mode
    ESP_ERROR_CHECK(ads111x_set_data_rate(&devices, ADS111X_DATA_RATE_32)); // 32 samples per second
    ESP_ERROR_CHECK(ads111x_set_input_mux(&devices, ADS111X_MUX_3_GND));    // positive = AIN0, negative = GND
    ESP_ERROR_CHECK(ads111x_set_gain(&devices, GAIN));
    uint8_t t ;  
	int32_t raw = 0;
	int buf[4];
	
    for (t=0;t<4;t++)
    {
        if (ads111x_get_value(&devices, (int16_t*)&raw) == ESP_OK)
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
    uint32_t avgValue =0;
	for(int i = 1; i < 3 ; i ++){
		avgValue+= buf[i];
	}
	avgValue  /= 2;
    printf("ADC : %ld\n",avgValue);
    float voltage = (float)avgValue *3.3/26400.0;
    float ntc_res = res * voltage / (3.3 - voltage);
    float Temp = 1.0 / (1.0 / t0 + log(ntc_res / res)/beta) - 273.0 ;
    return Temp;
	
}
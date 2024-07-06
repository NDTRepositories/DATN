idf_component_register(SRCS "Gateway.c"
           REQUIRES driver log datamanager json http lora gps nvs_flash vfs spiffs tft_library ds3231 i2cdev 
                    INCLUDE_DIRS ".")

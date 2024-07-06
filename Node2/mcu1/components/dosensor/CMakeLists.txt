idf_component_register(SRCS "dosensor.c"
                    REQUIRES driver log nvs_interface nvs_flash i2cdev ads1115
                    INCLUDE_DIRS "include")

idf_component_register(SRCS "ecsensor.c"
                    REQUIRES driver log nvs_interface i2cdev ads1115
                    INCLUDE_DIRS "include")

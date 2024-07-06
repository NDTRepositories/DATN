idf_component_register(SRCS "ads1115.c"
                    REQUIRES driver i2cdev log esp_idf_lib_helpers
                    INCLUDE_DIRS "include")

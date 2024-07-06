idf_component_register(SRCS "nvs_interface.c"
                  REQUIRES driver log nvs_flash 
                    INCLUDE_DIRS "include")

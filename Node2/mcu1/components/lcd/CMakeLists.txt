idf_component_register(SRCS "lcd.c"
         REQUIRES driver log ili9340 main
                    INCLUDE_DIRS "include")

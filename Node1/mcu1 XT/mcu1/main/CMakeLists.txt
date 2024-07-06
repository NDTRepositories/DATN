# set(srcs "main.c" "decode_png.c" "pngle.c")

# idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")

idf_component_register(SRCS "main.c" "decode_png.c" "pngle.c" "fontx.c" "ili9340.c" "LCD.c" "ADS1115.c" "myTemp.c" "DS3231.c" "GPS.c" "SDCard.c" "nvs_interface.c" "myDO.c" "myEC.c" "mypH.c" "i2cdev.c" "datamanager.c" "UartComunication.c"
                    INCLUDE_DIRS ".")
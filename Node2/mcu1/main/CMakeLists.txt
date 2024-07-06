set(srcs "main.c" "decode_png.c" "pngle.c")

idf_component_register(SRCS ${srcs} 
REQUIRES driver datamanager UartComunication  nvs_flash vfs spiffs gps ili9340 sdcard phsensor ecsensor dosensor temperature ads1115 i2cdev ds3231 lcd bmpfile
INCLUDE_DIRS ".")

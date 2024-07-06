idf_component_register(SRCS 
				"decode_png.c"
				"pngle.c"
				"8080_lcd_esp32s3.c"
				"fontx.c"
				"i2s_lcd_esp32_driver.c"
				"ili9341.c"
				"ili9486.c"
				"lcd_lib.c"
				"lcd_com.c"
				"LCD.c"
                 REQUIRES driver log nvs_flash vfs spiffs
                  INCLUDE_DIRS ".")
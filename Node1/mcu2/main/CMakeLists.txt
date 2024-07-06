idf_component_register(SRCS "mcu2.c"
REQUIRES driver log http nvs_flash tcp lora wifipeertopeer UartComunication esp_event esp_netif esp_wifi datamanager
INCLUDE_DIRS ".")

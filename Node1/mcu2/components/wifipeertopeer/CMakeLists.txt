idf_component_register(SRCS "wifipeertopeer.c"
                  PRIV_REQUIRES lora datamanager esp_event driver log nvs_flash esp_netif esp_wifi UartComunication 
                    INCLUDE_DIRS "include")

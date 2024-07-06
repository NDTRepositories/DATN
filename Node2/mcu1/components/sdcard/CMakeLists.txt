idf_component_register(SRCS "sdcard.c"
            REQUIRES driver log spi_flash sdmmc fatfs vfs
                    INCLUDE_DIRS "include")

idf_component_register(SRCS "http.c"
                    REQUIRES driver log UartComunication datamanager
                    INCLUDE_DIRS "include")

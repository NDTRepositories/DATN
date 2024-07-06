set(component_srcs "lora.c")
idf_component_register(SRCS "${component_srcs}"
                       PRIV_REQUIRES driver log 
                       INCLUDE_DIRS "include")

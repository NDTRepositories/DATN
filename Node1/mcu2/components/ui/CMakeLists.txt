idf_component_register(SRCS "ui_comp_hook.c" "ui_helpers.c" "ui.c"
                PRIV_REQUIRES driver log
                    INCLUDE_DIRS "include")

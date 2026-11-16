idf_component_register(SRCS "main.c" "bmp390.c" "ltr390.c" "sht45.c" "veml7700.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_i2c)

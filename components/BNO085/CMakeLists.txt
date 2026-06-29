idf_component_register(SRCS "BNO085.c"
					REQUIRES esp_driver_i2c
					REQUIRES esp_driver_gpio
                    INCLUDE_DIRS "include")

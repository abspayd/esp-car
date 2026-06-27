idf_component_register(SRCS "platform.c"
							"vl53l7cx_api.c"
						REQUIRES
							esp_driver_i2c
                    INCLUDE_DIRS "include")

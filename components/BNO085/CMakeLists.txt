idf_component_register(SRCS "BNO085.c"
							"sh2_hal.c"
							"sh2/euler.c"
							"sh2/sh2.c"
							"sh2/sh2_SensorValue.c"
							"sh2/sh2_util.c"
							"sh2/shtp.c"
					REQUIRES esp_driver_spi
					REQUIRES esp_timer
					REQUIRES esp_driver_gpio
					INCLUDE_DIRS "sh2"
					INCLUDE_DIRS "include"
				)

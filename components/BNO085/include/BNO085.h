#ifndef BNO085_H
#define BNO085_H

#include "driver/spi_master.h"
#include "sh2_hal.h"
#include <math.h>
#include <stdint.h>

#define BNO085_REQUEST_TIMEOUT_MS CONFIG_BNO085_REQUEST_TIMEOUT_MS

#define BNO085_SPI_CSN_GPIO CONFIG_BNO085_CSN_GPIO
#define BNO085_RESET_GPIO CONFIG_BNO085_RESET_GPIO
#define BNO085_INTERRUPT_GPIO CONFIG_BNO085_INTERRUPT_GPIO
#define BNO085_WAKE_GPIO CONFIG_BNO085_WAKE_GPIO

#define BNO085_CHANNEL_CMD 0
#define BNO085_CHANNEL_EXEC 1
#define BNO085_CHANNEL_SENSOR_HUB_CTRL 2
#define BNO085_CHANNEL_INPUT_SENSOR_REPORTS 3
#define BNO085_CHANNEL_WAKE_SENSOR_REPORTS 4
#define BNO085_CHANNEL_GYRO_ROTATION_VECTOR 5

extern void BNO085_Init(void);

extern double_t BNO085_Read_Gyro(void);
extern double_t BNO085_Read_Accelerometer(void);
extern double_t BNO085_Read_Magnetometer(void);

#endif

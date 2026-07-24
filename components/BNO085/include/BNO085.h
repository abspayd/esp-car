#ifndef BNO085_H
#define BNO085_H

#include "driver/spi_master.h"
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

typedef struct {
    spi_device_handle_t dev_handle;
} bno085_config_t;

/**
 * @brief BNO085_Init initializes the connection to a BNO085 device
 * @return (uint8_t) status: 0 if OK
 */
extern uint8_t BNO085_Init(bno085_config_t config);

extern void BNO085_Reset(void);

extern uint8_t BNO085_Read(bno085_config_t cfg, uint8_t *buf, size_t capacity,
                           size_t *length);
extern uint8_t BNO085_Write(bno085_config_t cfg, uint8_t *buf, size_t len);

#endif

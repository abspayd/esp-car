#ifndef BNO085_H
#define BNO085_H

#include "driver/i2c_types.h"
#include <stdint.h>

#define BNO085_REQUEST_TIMEOUT_MS CONFIG_BNO085_REQUEST_TIMEOUT_MS

#define BNO085_CHANNEL_CMD 0
#define BNO085_CHANNEL_EXEC 1
#define BNO085_CHANNEL_SENSOR_HUB_CTRL 2
#define BNO085_CHANNEL_INPUT_SENSOR_REPORTS 3
#define BNO085_CHANNEL_WAKE_SENSOR_REPORTS 4
#define BNO085_CHANNEL_GYRO_ROTATION_VECTOR 5

typedef struct {
    i2c_master_dev_handle_t dev_handle;
    uint8_t address;
} bno085_config_t;

/**
 * @brief BNO085_Init initializes the connection to a BNO085 device
 * @return (uint8_t) status: 0 if OK
 */
extern uint8_t BNO085_Init(bno085_config_t config);

#endif

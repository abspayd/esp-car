/**
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "platform.h"
#include "driver/i2c_master.h"
#include "freertos/idf_additions.h"

uint8_t VL53L7CX_RdByte(VL53L7CX_Platform *p_platform, uint16_t RegisterAdress,
                        uint8_t *p_value) {
    uint8_t status = 0;

    // esp_err_t error = i2c_master_receive(p_platform->dev_handle, p_value, 1,
    //                                      p_platform->timeout_ms);
    uint8_t buf[2] = {
        (uint8_t)(RegisterAdress >> 8),
        (uint8_t)(RegisterAdress & 0xFF),
    };

    esp_err_t error = i2c_master_transmit_receive(
        p_platform->dev_handle, buf, 2, p_value, 1, p_platform->timeout_ms);
    if (error != ESP_OK) {
        status |= 1;
    }

    return status;
}

uint8_t VL53L7CX_WrByte(VL53L7CX_Platform *p_platform, uint16_t RegisterAdress,
                        uint8_t value) {
    uint8_t status = 0;

    uint8_t buf[3] = {
        (uint8_t)(RegisterAdress >> 8),
        (uint8_t)(RegisterAdress & 0xFF),
        value,
    };

    esp_err_t error = i2c_master_transmit(p_platform->dev_handle, buf, 3,
                                          p_platform->timeout_ms);
    if (error != ESP_OK) {
        status |= 1;
    }

    return status;
}

uint8_t VL53L7CX_WrMulti(VL53L7CX_Platform *p_platform, uint16_t RegisterAdress,
                         uint8_t *p_values, uint32_t size) {
    uint8_t status = 0;

    uint8_t *buf = malloc(size + 2);
    if (buf == NULL) {
        return 1;
    }

    buf[0] = (uint8_t)(RegisterAdress >> 8);
    buf[1] = (uint8_t)(RegisterAdress & 0xFF);
    memcpy(&buf[2], p_values, size);

    esp_err_t error = i2c_master_transmit(p_platform->dev_handle, buf, size + 2,
                                          p_platform->timeout_ms);

    free(buf);

    if (error != ESP_OK) {
        status |= 1;
    }

    return status;
}

uint8_t VL53L7CX_RdMulti(VL53L7CX_Platform *p_platform, uint16_t RegisterAdress,
                         uint8_t *p_values, uint32_t size) {
    uint8_t status = 0;

    uint8_t buf[2] = {
        (uint8_t)(RegisterAdress >> 8),
        (uint8_t)(RegisterAdress & 0xFF),
    };
    esp_err_t error = i2c_master_transmit_receive(
        p_platform->dev_handle, buf, 2, p_values, size, p_platform->timeout_ms);
    if (error != ESP_OK) {
        status |= 1;
    }

    return status;
}

uint8_t VL53L7CX_Reset_Sensor(VL53L7CX_Platform *p_platform) {
    uint8_t status = 0;

    /* (Optional) Need to be implemented by customer. This function returns 0 if
     * OK */

    /* Set pin LPN to LOW */
    /* Set pin AVDD to LOW */
    /* Set pin VDDIO  to LOW */
    VL53L7CX_WaitMs(p_platform, 100);

    /* Set pin LPN of to HIGH */
    /* Set pin AVDD of to HIGH */
    /* Set pin VDDIO of  to HIGH */
    VL53L7CX_WaitMs(p_platform, 100);

    return status;
}

void VL53L7CX_SwapBuffer(uint8_t *buffer, uint16_t size) {
    uint32_t i, tmp;

    /* Example of possible implementation using <string.h> */
    for (i = 0; i < size; i = i + 4) {
        tmp = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) |
              (buffer[i + 3]);

        memcpy(&(buffer[i]), &tmp, 4);
    }
}

uint8_t VL53L7CX_WaitMs(VL53L7CX_Platform *p_platform, uint32_t TimeMs) {

    vTaskDelay(TimeMs / portTICK_PERIOD_MS);

    return 0;
}

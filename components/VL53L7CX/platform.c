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
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/idf_additions.h"

uint8_t VL53L7CX_RdByte(VL53L7CX_Platform *p_platform, uint16_t RegisterAdress,
                        uint8_t *p_value) {
    uint8_t status = 0;

    uint8_t buf[2] = {
        (uint8_t)(RegisterAdress >> 8),
        (uint8_t)(RegisterAdress & 0xFF),
    };

    esp_err_t error =
        i2c_master_transmit_receive(p_platform->dev_handle, buf, 2, p_value, 1,
                                    VL53L7CX_REQUEST_TIMEOUT_MS);
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
                                          VL53L7CX_REQUEST_TIMEOUT_MS);
    if (error != ESP_OK) {
        status |= 1;
    }

    return status;
}

uint8_t VL53L7CX_WrMulti(VL53L7CX_Platform *p_platform, uint16_t RegisterAdress,
                         uint8_t *p_values, uint32_t size) {
    uint8_t status = 0;

    const uint32_t CHUNK_SIZE = 256;
    uint32_t remaining = size;
    uint8_t *p = p_values;
    uint16_t reg = RegisterAdress;

    while (remaining > 0) {
        uint32_t n = (CHUNK_SIZE < remaining) ? CHUNK_SIZE : remaining;

        i2c_master_transmit_multi_buffer_info_t buf[2];

        uint8_t addr[2] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF)};
        buf[0].buffer_size = 2;
        buf[0].write_buffer = addr;

        buf[1].buffer_size = n;
        buf[1].write_buffer = p;

        status = i2c_master_multi_buffer_transmit(
            p_platform->dev_handle, buf, 2, VL53L7CX_REQUEST_TIMEOUT_MS);

        if (status) {
            return 1;
        }

        remaining -= n;
        p += n;
        reg = (uint16_t)(reg + n);
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
    esp_err_t error =
        i2c_master_transmit_receive(p_platform->dev_handle, buf, 2, p_values,
                                    size, VL53L7CX_REQUEST_TIMEOUT_MS);
    if (error != ESP_OK) {
        status |= 1;
    }

    return status;
}

uint8_t VL53L7CX_Reset_Sensor(VL53L7CX_Platform *p_platform) {
    uint8_t status = 0;

    /* (Optional) Need to be implemented by customer. This function returns 0 if
     * OK */

    gpio_set_direction(VL53L7CX_LPN_GPIO_NUM, GPIO_MODE_OUTPUT);

    /* Set pin LPN to LOW */
    gpio_set_level(VL53L7CX_LPN_GPIO_NUM, 0);
    /* Set pin AVDD to LOW */
    /* Set pin VDDIO  to LOW */
    VL53L7CX_WaitMs(p_platform, 100);

    /* Set pin LPN of to HIGH */
    gpio_set_level(VL53L7CX_LPN_GPIO_NUM, 1);
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

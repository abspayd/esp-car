#include "BNO085.h"
#include "driver/i2c_master.h"
#include <stdio.h>

uint8_t BNO085_Init(bno085_config_t device_config) {
    uint8_t status = 0;

    uint8_t buf[4] = {0x00, 0x00, 0x00, 0x00};
    // uint8_t data[20];
    uint8_t size;
    i2c_master_transmit_receive(device_config.dev_handle, buf, 4, &size, 1,
                                BNO085_REQUEST_TIMEOUT_MS);

    printf("raw size: 0x%02x (%u)\n", size, size);
    size = (size >> 4) | (size << 4);
    printf("flipped size: 0x%02x (%u)\n", size, size);
    uint8_t *payload = malloc(size);
    i2c_master_receive(device_config.dev_handle, payload, size,
                       BNO085_REQUEST_TIMEOUT_MS);

    for (int i = 0; i < size; i++) {
        printf("0x%02x\n", payload[i]);
    }

    free(payload);

    return status;
}

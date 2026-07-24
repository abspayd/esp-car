#include "BNO085.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include <stdio.h>
#include <string.h>

uint8_t BNO085_Init(bno085_config_t device_config) {

    ESP_LOGI("BNO085_Init", "Interrupt pin: %d", BNO085_INTERRUPT_GPIO);

    gpio_set_direction(BNO085_INTERRUPT_GPIO, GPIO_MODE_INPUT);

    // rtc_gpio_init(BNO085_INTERRUPT_GPIO);

    // if (!rtc_gpio_is_valid_gpio(BNO085_INTERRUPT_GPIO)) {
    //     char *taskName = pcTaskGetName(NULL);
    //     ESP_LOGE(taskName, "BNO085 interrupt pin does not support RTC!");
    //     return 1;
    // }
    //
    // rtc_gpio_wakeup_enable(BNO085_INTERRUPT_GPIO, GPIO_INTR_LOW_LEVEL);

    return 0;
}

void BNO085_Reset(void) {

    gpio_set_direction(BNO085_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO085_WAKE_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_level(BNO085_WAKE_GPIO, 1);
    gpio_set_level(BNO085_RESET_GPIO, 0);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    gpio_set_level(BNO085_RESET_GPIO, 1);

    vTaskDelay(100 / portTICK_PERIOD_MS);
}

uint8_t BNO085_Read(bno085_config_t cfg, uint8_t *buf, size_t capacity,
                    size_t *length) {

    ESP_ERROR_CHECK(spi_device_acquire_bus(cfg.dev_handle, portMAX_DELAY));

    gpio_set_direction(BNO085_INTERRUPT_GPIO, GPIO_MODE_INPUT);

    for (;;) {
        if (gpio_get_level(BNO085_INTERRUPT_GPIO) == 0) {
            break;
        }
        vTaskDelay(3 / portTICK_PERIOD_MS);
    }

    ESP_LOGI("BNO085_Read", "Got interrupt");

    uint8_t shtp_header[4] = {0};
    shtp_header[0] = 0x04;
    spi_transaction_t t = {
        .length = capacity * 8,
        .tx_buffer = shtp_header,
        .rx_buffer = buf,
    };
    esp_err_t err = spi_device_polling_transmit(cfg.dev_handle, &t);
    if (err != ESP_OK) {
        ESP_LOGE("BNO085_Read", "Failed to transmit packet");
        spi_device_release_bus(cfg.dev_handle);
        return 1;
    }

    uint16_t size = ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
    bool continuation = (size & 0x8000) != 0;
    size = size & 0x7FFF;

    if (capacity < size) {
        ESP_LOGE("BNO085_Read", "Packet size %zu exceeds capacity %zu", size,
                 capacity);
        spi_device_release_bus(cfg.dev_handle);
        return 1;
    }

    printf("Size: %zu, continuation: %d\n", size, continuation);
    for (int i = 0; i < size; i++) {
        printf("0x%02x (%c)\n", buf[i], (char)buf[i]);
    }

    spi_device_release_bus(cfg.dev_handle);

    return 0;
}

uint8_t BNO085_Write(bno085_config_t cfg, uint8_t *buf, size_t len) {

    esp_err_t err = spi_device_acquire_bus(cfg.dev_handle, portMAX_DELAY);
    if (err != ESP_OK) {
        return 1;
    }

    uint8_t shtp_header[4] = {0x04, 0x00, 0x00, 0x00};
    uint8_t response[4];
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = shtp_header;
    t.rxlength = 4;
    t.rx_buffer = response;
    ESP_ERROR_CHECK(spi_device_transmit(cfg.dev_handle, &t));

    for (int i = 0; i < 4; i++) {
        printf("0x%02x\n", ((uint8_t *)t.rx_buffer)[i]);
    }

    uint16_t response_length =
        ((uint16_t)response[0]) + ((uint16_t)response[1] << 8);
    return 0;
}

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/i2c_types.h"
#include "portmacro.h"
#include <stdio.h>

#define LED_GPIO CONFIG_LED_GPIO
#define ONBOARD_LED_GPIO CONFIG_ONBOARD_LED_GPIO

#define I2C_MASTER_SDA_GPIO GPIO_NUM_5
#define I2C_MASTER_SCL_GPIO GPIO_NUM_6
#define BNO085_HOST_INTN GPIO_NUM_43
#define BNO058_ADDRESS 0x4A

#define VL53L7CX_ADDRESS 0x52 >> 1

typedef struct {
    uint32_t delay;
    uint32_t gpio_pin;
} blink_config_t;

static void blink_task(void *args) {
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Beginning blink task on GPIO pin %d", LED_GPIO);

    blink_config_t *config = (blink_config_t *)args;

    gpio_set_direction(config->gpio_pin, GPIO_MODE_OUTPUT);

    for (;;) {
        uint32_t core_id = xPortGetCoreID();
        ESP_LOGI(taskName, "Current core: %d", core_id);

        vTaskDelay((TickType_t)config->delay);
        gpio_set_level(config->gpio_pin, 1);
        vTaskDelay((TickType_t)config->delay);
        gpio_set_level(config->gpio_pin, 0);
    }
}

void app_main(void) {
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Start");

    blink_config_t blink_config_1 = {
        .delay = 500 / portTICK_PERIOD_MS,
        .gpio_pin = ONBOARD_LED_GPIO,
    };
    xTaskCreate(blink_task, "led1", 2048, (void *)&blink_config_1, 1, NULL);

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_GPIO,
        .sda_io_num = I2C_MASTER_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    gpio_set_direction(BNO085_HOST_INTN, GPIO_MODE_INPUT);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L7CX_ADDRESS,
        .scl_speed_hz = 100 * 1000,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    ESP_ERROR_CHECK(i2c_master_probe(bus_handle, VL53L7CX_ADDRESS, 100));

    uint8_t buf[20];
    memset(buf, 0, 20);
    i2c_master_receive(dev_handle, buf, 20, -1);

    for (int i = 0; i < 20; i++) {
        printf("0x%02x\n", buf[i]);
    }

    // i2c_device_config_t dev_cfg = {
    //     .dev_addr_length = 7,
    //     .device_address = BNO058_ADDRESS,
    //     .scl_speed_hz = 100000,
    // };
    // i2c_master_dev_handle_t dev_handle;
    // ESP_ERROR_CHECK(
    //     i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    //
    // ESP_ERROR_CHECK(i2c_master_probe(bus_handle, BNO058_ADDRESS, 5000));
    //
    // i2c_master_bus_reset(bus_handle);
    //
    // for (;;) {
    //     while (gpio_get_level(BNO085_HOST_INTN) == 1) {
    //         ESP_LOGI(taskName, "Waiting for interrupt");
    //         vTaskDelay(100);
    //     }
    //     ESP_LOGI(taskName, "Got interrupt!");
    //
    //     // ESP_ERROR_CHECK(i2c_master_probe(bus_handle, BNO058_ADDRESS,
    //     100));
    //
    //     // ESP_LOGI(taskName, "Found device 0x%02X", BNO058_ADDRESS);
    //
    //     // TODO: figure out how to communicate with BNO058
    //     uint8_t buf[100];
    //     memset(buf, 0, 100);
    //     i2c_master_receive(dev_handle, buf, 2, -1);
    //     uint16_t package_length = ((uint16_t)buf[0]) & 0xEF;
    //
    //     printf("Package length: %d\n", package_length);
    //     i2c_master_receive(dev_handle, buf, package_length, -1);
    //
    //     for (int i = 0; i < package_length; i++) {
    //         printf("0x%02x\n", buf[i]);
    //     }
    // }

    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));

    vTaskSuspend(NULL);
}

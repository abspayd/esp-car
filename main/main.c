#include "BNO085.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "hal/i2c_types.h"
#include "platform.h"
#include "portmacro.h"
#include "vl53l7cx_api.h"
#include <stdio.h>

#define LED_GPIO CONFIG_LED_GPIO
#define ONBOARD_LED_GPIO CONFIG_ONBOARD_LED_GPIO

#define I2C_MASTER_SDA_GPIO GPIO_NUM_5
#define I2C_MASTER_SCL_GPIO GPIO_NUM_6
#define BNO085_HOST_INTN CONFIG_BNO085_INTERRUPT_GPIO
#define BNO058_ADDRESS 0x4A

#define VL53L7CX_ADDRESS 0x52 >> 1
#define VL53L7CX_INTERRUPT_GPIO CONFIG_VL53L7CX_INTERRUPT_GPIO

typedef struct {
    uint32_t delay;
    uint32_t gpio_pin;
} blink_config_t;

static VL53L7CX_Configuration vl53l7cx_dev;

static void blink_task(void *args) {
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Beginning blink task on GPIO pin %d", LED_GPIO);

    blink_config_t *config = (blink_config_t *)args;

    gpio_set_direction(config->gpio_pin, GPIO_MODE_OUTPUT);
    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);

    ESP_LOGI(taskName, "task stack high watermark: %d", uxHighWaterMark);

    for (;;) {
        vTaskDelay((TickType_t)config->delay);
        gpio_set_level(config->gpio_pin, 1);
        vTaskDelay((TickType_t)config->delay);
        gpio_set_level(config->gpio_pin, 0);
    }
}

static void tof_task(void *args) {
    char *taskName = pcTaskGetName(NULL);

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(taskName, "task stack high watermark: %d", uxHighWaterMark);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    uint8_t isAlive, status;
    VL53L7CX_Reset_Sensor(&vl53l7cx_dev.platform);

    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(taskName, "task stack high watermark: %d", uxHighWaterMark);

    status = vl53l7cx_is_alive(&vl53l7cx_dev, &isAlive);
    if (!isAlive || status) {
        printf("VL53L7CX not detected at requested address\n");
        vTaskDelete(NULL);
    }
    printf("Found VL53L7CX device!\n");

    status = vl53l7cx_init(&vl53l7cx_dev);
    if (status) {
        printf("Failed to init VL53L7CX\n");
        vTaskDelete(NULL);
    }
    printf("Initialized VL53L7CX!\n");

    status = vl53l7cx_set_resolution(&vl53l7cx_dev, VL53L7CX_RESOLUTION_8X8);
    if (status) {
        printf("Failed to set resolution\n");
        vTaskDelete(NULL);
    }
    printf("Updated resolution\n");

    uint8_t current_resolution;
    status = vl53l7cx_get_resolution(&vl53l7cx_dev, &current_resolution);
    if (status) {
        printf("Failed to get resolution\n");
        vTaskDelete(NULL);
    }
    printf("Resolution: %d\n", current_resolution);

    status = vl53l7cx_set_ranging_frequency_hz(&vl53l7cx_dev, 15);
    if (status) {
        printf("Failed to set ranging frequency\n");
        vTaskDelete(NULL);
    }

    status = vl53l7cx_start_ranging(&vl53l7cx_dev);
    if (status) {
        printf("Failed to start ranging session\n");
        vTaskDelete(NULL);
    }

    gpio_set_direction(VL53L7CX_INTERRUPT_GPIO, GPIO_MODE_INPUT);

    printf("\e[2J\e[H");
    for (;;) {

        if (gpio_get_level(VL53L7CX_INTERRUPT_GPIO) != 0) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            continue;
        }

        VL53L7CX_ResultsData results;
        vl53l7cx_get_ranging_data(&vl53l7cx_dev, &results);

        printf("\e[H");
        for (int i = 0; i < 64; i += 8) {
            for (int j = 0; j < 8; j++) {
                int k = (i + j) * VL53L7CX_NB_TARGET_PER_ZONE;
                int dist = results.target_status[k] == 5 ||
                                   results.target_status[k] == 9
                               ? results.distance_mm[k]
                               : -1;
                printf("%04d ", dist);
            }
            printf("\n");
        }
    }
}

void app_main(void) {
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Start");

    blink_config_t blink_config_1 = {
        .delay = 500 / portTICK_PERIOD_MS,
        .gpio_pin = ONBOARD_LED_GPIO,
    };
    xTaskCreate(blink_task, "led1", 1024 * 2, (void *)&blink_config_1, 1, NULL);

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

    i2c_device_config_t vl53l7cx_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L7CX_ADDRESS,
        .scl_speed_hz = 100 * 1000,
    };

    i2c_master_dev_handle_t vl53l7cx_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &vl53l7cx_dev_cfg,
                                              &vl53l7cx_handle));

    vl53l7cx_dev.platform = (VL53L7CX_Platform){
        .address = VL53L7CX_ADDRESS,
        .dev_handle = vl53l7cx_handle,
    };

    // xTaskCreate(tof_task, "tof", 1024 * 3, NULL, 2, NULL);

    i2c_device_config_t bno085_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BNO058_ADDRESS,
        .scl_speed_hz = 100 * 1000,
    };
    i2c_master_dev_handle_t bno085_handle;
    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(bus_handle, &bno085_dev_cfg, &bno085_handle));
    bno085_config_t bno085_dev = {
        .address = BNO058_ADDRESS,
        .dev_handle = bno085_handle,

    };
    BNO085_Init(bno085_dev);

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

    // ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    // ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));

    vTaskSuspend(NULL);
}

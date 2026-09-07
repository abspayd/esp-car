#include "BNO085.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "hal/i2c_types.h"
#include "platform.h"
#include "portmacro.h"
#include "sensors/imu.h"
#include "sensors/tof.h"
#include "vl53l7cx_api.h"
#include <stdio.h>

#define LED_GPIO CONFIG_LED_GPIO
#define ONBOARD_LED_GPIO CONFIG_ONBOARD_LED_GPIO

// I2C definitions
#define I2C_MASTER_SDA_GPIO GPIO_NUM_5
#define I2C_MASTER_SCL_GPIO GPIO_NUM_6

// SPI definitions
#define SPI_MOSI_GPIO CONFIG_SPI_MOSI_GPIO
#define SPI_MISO_GPIO CONFIG_SPI_MISO_GPIO
#define SPI_SCK_GPIO CONFIG_SPI_SCK_GPIO

typedef struct {
    uint32_t delay;
    uint32_t gpio_pin;
} blink_config_t;

VL53L7CX_Configuration vl53l7cx_dev;

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
        .device_address = VL53L7CX_DEFAULT_I2C_ADDRESS >> 1,
        .scl_speed_hz = 100 * 1000,
    };

    vl53l7cx_dev.platform = (VL53L7CX_Platform){
        .address = VL53L7CX_DEFAULT_I2C_ADDRESS >> 1,
        .dev_handle = NULL,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &vl53l7cx_dev_cfg, &vl53l7cx_dev.platform.dev_handle));

    // TODO: move IMU stuff to its own task
    //	- This stuff is getting timed out, and I'm guessing it's because
    //	it's hijacking the main task.
    // BNO085_Init();
    // TODO: try to read data from the IMU
    // BNO085_Read_Accelerometer();

    // xTaskCreate(tof_task, "tof", 1024 * 3, NULL, 2, NULL);
    // imu_task(NULL);
    xTaskCreate(imu_task, "imu", 1024 * 2, NULL, 2, NULL);

    vTaskSuspend(NULL);
}

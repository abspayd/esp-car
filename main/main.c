#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include <stdio.h>

#define LED_GPIO CONFIG_LED_GPIO
#define ONBOARD_LED_GPIO CONFIG_ONBOARD_LED_GPIO

static void blink_task(void *delay) {
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Beginning blink task on GPIO pin %d", LED_GPIO);

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    for (;;) {
        vTaskDelay((TickType_t)delay);
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay((TickType_t)delay);
        gpio_set_level(LED_GPIO, 0);
    }
}

void app_main(void) {
    char *taskName = pcTaskGetName(NULL);
    ESP_LOGI(taskName, "Start");

    TaskHandle_t xHandle = NULL;
    BaseType_t xReturned =
        xTaskCreate(blink_task, "BLINK", 2048,
                    (void *)(250 / portTICK_PERIOD_MS), 0, &xHandle);

    gpio_set_direction(ONBOARD_LED_GPIO, GPIO_MODE_OUTPUT);
    uint32_t delay = 100 / portTICK_PERIOD_MS;
    for (;;) {
        vTaskDelay((TickType_t)delay);
        gpio_set_level(ONBOARD_LED_GPIO, 1);
        vTaskDelay((TickType_t)delay);
        gpio_set_level(ONBOARD_LED_GPIO, 0);
    }
}

#include "sensors/tof.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "platform.h"
#include "portmacro.h"
#include "vl53l7cx_api.h"

extern void tof_task(void *args) {
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
                int dist = results.target_status[k] == 5 || results.target_status[k] == 9 ? results.distance_mm[k] : -1;
                printf("%04d ", dist);
            }
            printf("\n");
        }
    }
}

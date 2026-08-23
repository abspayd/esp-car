#include "BNO085.h"
#include "esp_log.h"
#include "sh2.h"
#include "sh2_spi.h"
#include <string.h>

void test_callback(void *cookie, sh2_AsyncEvent_t *pEvent) {
    // TODO
    ESP_LOGI("test_callback", "test");
}

void BNO085_Init(void) {
    int err = sh2_open(&sh2_hal, test_callback, NULL);
    if (err != 0) {
        ESP_LOGE("app_main", "Failed to initialize BNO085: %d", err);
        return;
    }

    // Enable the sensors
    sh2_SensorConfig_t sensor_cfg = {
        .alwaysOnEnabled = true,
    };

    err = sh2_setSensorConfig(SH2_GYROSCOPE_CALIBRATED, &sensor_cfg);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Error enabling gyroscope: %d", err);
        return;
    }
    sh2_setSensorConfig(SH2_ACCELEROMETER, &sensor_cfg);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Error enabling accelerometer: %d", err);
        return;
    }
    sh2_setSensorConfig(SH2_MAGNETIC_FIELD_CALIBRATED, &sensor_cfg);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Error enabling magnetometer: %d", err);
        return;
    }

    ESP_LOGI("app_main", "Success!");
}

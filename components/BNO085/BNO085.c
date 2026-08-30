#include "BNO085.h"
#include "esp_log.h"
#include "sh2.h"
#include "sh2_spi.h"
#include <string.h>

void eventHandler(void *cookie, sh2_AsyncEvent_t *pEvent) {
    // TODO
    ESP_LOGI("eventHandler", "Event handler: %d", pEvent->eventId);
    if (pEvent->eventId == SH2_RESET) {
        ESP_LOGI("EventHandler", "RESET EVENT");
    } else if (pEvent->eventId == SH2_SHTP_EVENT) {
        ESP_LOGI("EventHandler", "SHTP EVENT: %d", pEvent->shtpEvent);
    }
}

sh2_SensorCallback_t foo;
void sensorHandler(void *cookie, sh2_SensorEvent_t *sEvent) { ESP_LOGI("sensor_callback", "Sensor event"); }

void BNO085_Init(void) {
    int err = sh2_open(&sh2_hal, eventHandler, NULL);
    if (err != 0) {
        ESP_LOGE("app_main", "Failed to initialize BNO085: %d", err);
        return;
    }
    // Enable the sensors
    sh2_SensorConfig_t sensor_cfg = {
        .reportInterval_us = 10000,
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

    sh2_setSensorCallback(sensorHandler, NULL);

    sh2_ProductIds_t prod_ids;
    memset(&prod_ids, 0, sizeof(prod_ids));
    err = sh2_getProdIds(&prod_ids);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Failed to get prod ids: ", err);
        return;
    }

    ESP_LOGI("app_main", "Success!");
}

double_t BNO085_Read_Accelerometer() { return -1.0; }

#include "BNO085.h"
#include "esp_log.h"
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "sh2_spi.h"
#include <stdio.h>

sh2_Hal_t *sh2_hal = 0;

sh2_SensorValue_t raw_accelerometer_value;
sh2_SensorValue_t raw_gyroscope_value;

sh2_SensorValue_t accelerometer_value;
sh2_SensorValue_t gyroscope_value;
sh2_SensorValue_t magnetometer_value;

void eventHandler(void *cookie, sh2_AsyncEvent_t *pEvent) {
    if (pEvent->eventId == SH2_RESET) {
        // ESP_LOGI("EventHandler", "RESET EVENT");
    } else if (pEvent->eventId == SH2_SHTP_EVENT) {
        ESP_LOGI("EventHandler", "SHTP EVENT: %d", pEvent->shtpEvent);
    }
}

void sensorHandler(void *cookie, sh2_SensorEvent_t *sEvent) {
    // ESP_LOGI("sensor_callback", "Sensor event");

    sh2_SensorValue_t value;
    int rc = sh2_decodeSensorEvent(&value, sEvent);
    if (rc != SH2_OK) {
        ESP_LOGE("sensorHandler", "Error decoding sensor event: %d\n", rc);
        return;
    }

    // float scaleRadToDeg = 180.0 / 3.14159265358;
    // float r, i, j, k, acc_deg, x, y, z, t;
    // static int skip = 0;
    // t = value.timestamp / 1000000.0; // time in seconds.
    switch (value.sensorId) {
    case SH2_LINEAR_ACCELERATION:
        accelerometer_value = value;
        break;
    case SH2_GYROSCOPE_CALIBRATED:
        gyroscope_value = value;
        break;
    case SH2_MAGNETIC_FIELD_CALIBRATED:
        magnetometer_value = value;
        break;
    default:
        printf("Unknown sensor: %d\n", value.sensorId);
        break;
    }
}

void BNO085_Init(void) {
    sh2_hal = sh2_hal_init();

    int err = sh2_open(sh2_hal, eventHandler, NULL);
    if (err != 0) {
        ESP_LOGE("app_main", "Failed to initialize BNO085: %d", err);
        return;
    }

    sh2_setSensorCallback(sensorHandler, NULL);

    // xTaskCreate(service_task, "bno085_service_task", 1024, NULL, 10, NULL);

    // printf("Trying to read prod ids\n");
    // sh2_ProductIds_t prod_ids;
    // memset(&prod_ids, 0, sizeof(prod_ids));
    // err = sh2_getProdIds(&prod_ids);
    // if (err != 0) {
    //     ESP_LOGE("BNO085_Init", "Failed to get prod ids: ", err);
    //     return;
    // }
    //
    // ESP_LOGI("BNO085_Init", "prod id entry count: %d", prod_ids.numEntries);
    //
    // // for (int i = 0; i < prod_ids.numEntries; i++) {
    // //     printf("part number: %ld\n", prod_ids.entry[i].swPartNumber);
    // // }
    // for (int n = 0; n < prod_ids.numEntries; n++) {
    //     printf("Part %ld : Version %d.%d.%d Build %ld\n", prod_ids.entry[n].swPartNumber,
    //            prod_ids.entry[n].swVersionMajor, prod_ids.entry[n].swVersionMinor, prod_ids.entry[n].swVersionPatch,
    //            prod_ids.entry[n].swBuildNumber);
    //
    //     // Wait a bit so we don't overflow the console output.
    // }

    // Enable the sensors
    sh2_SensorConfig_t sensor_cfg = {
        .reportInterval_us = 10000,
        .alwaysOnEnabled = true,
    };

    sh2_setSensorConfig(SH2_LINEAR_ACCELERATION, &sensor_cfg);
    sh2_setSensorConfig(SH2_GYROSCOPE_CALIBRATED, &sensor_cfg);
    sh2_setSensorConfig(SH2_MAGNETIC_FIELD_CALIBRATED, &sensor_cfg);

    // uint64_t start_ms = esp_timer_get_time() * 1000;
    // for (;;) {
    //     sh2_service();
    //     // uint64_t now_ms = esp_timer_get_time() * 1000;
    //     // if (now_ms - start_ms > 1000) {
    //     //     printf("service loop\n");
    //     // }
    // }

    // sh2_close();
    return;
}

sh2_SensorValue_t BNO085_Read_Accelerometer() { return accelerometer_value; }
sh2_SensorValue_t BNO085_Read_Gyroscope() { return gyroscope_value; }
sh2_SensorValue_t BNO085_Read_Magnetometer() { return magnetometer_value; }

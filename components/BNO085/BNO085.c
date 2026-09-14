#include "BNO085.h"
#include "esp_log.h"
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "sh2_spi.h"
#include <string.h>

sh2_Hal_t *sh2_hal = 0;

void eventHandler(void *cookie, sh2_AsyncEvent_t *pEvent) {
    // TODO
    ESP_LOGI("eventHandler", "Event handler: %d", pEvent->eventId);
    if (pEvent->eventId == SH2_RESET) {
        ESP_LOGI("EventHandler", "RESET EVENT");
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
    float scaleRadToDeg = 180.0 / 3.14159265358;
    float r, i, j, k, acc_deg, x, y, z, t;
    static int skip = 0;
    t = value.timestamp / 1000000.0; // time in seconds.
    switch (value.sensorId) {
    case SH2_RAW_ACCELEROMETER:
        printf("%8.4f Raw acc: %d %d %d time_us:%ld\n", (double)t, value.un.rawAccelerometer.x,
               value.un.rawAccelerometer.y, value.un.rawAccelerometer.z, value.un.rawAccelerometer.timestamp);
        break;

    case SH2_ACCELEROMETER:
        printf("%8.4f Acc: %f %f %f\n", (double)t, (double)value.un.accelerometer.x, (double)value.un.accelerometer.y,
               (double)value.un.accelerometer.z);
        break;

    case SH2_RAW_GYROSCOPE:
        printf("%8.4f Raw gyro: x:%d y:%d z:%d temp:%d time_us:%ld\n", (double)t, value.un.rawGyroscope.x,
               value.un.rawGyroscope.y, value.un.rawGyroscope.z, value.un.rawGyroscope.temperature,
               value.un.rawGyroscope.timestamp);
        break;

    case SH2_ROTATION_VECTOR:
        r = value.un.rotationVector.real;
        i = value.un.rotationVector.i;
        j = value.un.rotationVector.j;
        k = value.un.rotationVector.k;
        acc_deg = scaleRadToDeg * value.un.rotationVector.accuracy;
        printf("%8.4f Rotation Vector: "
               "r:%0.6f i:%0.6f j:%0.6f k:%0.6f (acc: %0.6f deg)\n",
               (double)t, (double)r, (double)i, (double)j, (double)k, (double)acc_deg);
        break;
    case SH2_GAME_ROTATION_VECTOR:
        r = value.un.gameRotationVector.real;
        i = value.un.gameRotationVector.i;
        j = value.un.gameRotationVector.j;
        k = value.un.gameRotationVector.k;
        printf("%8.4f GRV: "
               "r:%0.6f i:%0.6f j:%0.6f k:%0.6f\n",
               (double)t, (double)r, (double)i, (double)j, (double)k);
        break;
    case SH2_GYROSCOPE_CALIBRATED:
        x = value.un.gyroscope.x;
        y = value.un.gyroscope.y;
        z = value.un.gyroscope.z;
        printf("%8.4f GYRO: "
               "x:%0.6f y:%0.6f z:%0.6f\n",
               (double)t, (double)x, (double)y, (double)z);
        break;
    case SH2_GYROSCOPE_UNCALIBRATED:
        x = value.un.gyroscopeUncal.x;
        y = value.un.gyroscopeUncal.y;
        z = value.un.gyroscopeUncal.z;
        printf("%8.4f GYRO_UNCAL: "
               "x:%0.6f y:%0.6f z:%0.6f\n",
               (double)t, (double)x, (double)y, (double)z);
        break;
    case SH2_GYRO_INTEGRATED_RV:
        // These come at 1kHz, too fast to print all of them.
        // So only print every 10th one
        skip++;
        if (skip == 10) {
            skip = 0;
            r = value.un.gyroIntegratedRV.real;
            i = value.un.gyroIntegratedRV.i;
            j = value.un.gyroIntegratedRV.j;
            k = value.un.gyroIntegratedRV.k;
            x = value.un.gyroIntegratedRV.angVelX;
            y = value.un.gyroIntegratedRV.angVelY;
            z = value.un.gyroIntegratedRV.angVelZ;
            printf("%8.4f Gyro Integrated RV: "
                   "r:%0.6f i:%0.6f j:%0.6f k:%0.6f x:%0.6f y:%0.6f z:%0.6f\n",
                   (double)t, (double)r, (double)i, (double)j, (double)k, (double)x, (double)y, (double)z);
        }
        break;
    case SH2_IZRO_MOTION_REQUEST:
        printf("IZRO Request: intent:%d, request:%d\n", value.un.izroRequest.intent, value.un.izroRequest.request);
        break;
    case SH2_SHAKE_DETECTOR:
        printf("Shake Axis: %c%c%c\n", (value.un.shakeDetector.shake & SHAKE_X) ? 'X' : '.',
               (value.un.shakeDetector.shake & SHAKE_Y) ? 'Y' : '.',
               (value.un.shakeDetector.shake & SHAKE_Z) ? 'Z' : '.');

        break;
    case SH2_STABILITY_CLASSIFIER:
        printf("Stability Classification: %d\n", value.un.stabilityClassifier.classification);
        break;
    case SH2_STABILITY_DETECTOR:
        printf("Stability Detector: %d\n", value.un.stabilityDetector.stability);
        break;
    case SH2_MAGNETIC_FIELD_CALIBRATED:
        printf("Magnetic field Detector: x:%0.6f, y:%0.6f, z:%0.6f\n", value.un.magneticField.x,
               value.un.magneticField.y, value.un.magneticField.z);
        break;
    default:
        printf("Unknown sensor: %d\n", value.sensorId);
        break;
    }
}

static void service_task(void *arg) {
    for (;;) {
        sh2_service();
        // vTaskDelay(1 / portTICK_PERIOD_MS);
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

    printf("Setting gyro config\n");
    err = sh2_setSensorConfig(SH2_GYROSCOPE_CALIBRATED, &sensor_cfg);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Error enabling gyroscope: %d", err);
        return;
    }
    printf("Setting accel config\n");
    sh2_setSensorConfig(SH2_ACCELEROMETER, &sensor_cfg);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Error enabling accelerometer: %d", err);
        return;
    }
    printf("Setting magnet config\n");
    sh2_setSensorConfig(SH2_MAGNETIC_FIELD_CALIBRATED, &sensor_cfg);
    if (err != 0) {
        ESP_LOGE("BNO085_Init", "Error enabling magnetometer: %d", err);
        return;
    }

    ESP_LOGI("BNO085_Init", "Success!");

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

double_t BNO085_Read_Accelerometer() {
    //

    // const sh2_Sensor

    // sh2_SensorValue_t value;
    // int rc = sh2_decodeSensorEvent(&value, event) return -1.0;
    return -1.0;
}

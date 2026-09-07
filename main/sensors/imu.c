#include "sensors/imu.h"
#include "BNO085.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"

static bool bno_initialized = false;

void imu_task(void *args) {
    if (!bno_initialized) {
        BNO085_Init();
    }

    for (;;) {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    // TODO
    // BNO085_Read_Gyro();
    // BNO085_Read_Accelerometer();
    // BNO085_Read_Magnetometer();
}

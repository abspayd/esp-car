#include "sensors/imu.h"
#include "BNO085.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "sh2.h"

static bool bno_initialized = false;

void imu_task(void *args) {
    if (!bno_initialized) {
        BNO085_Init();
    }

    for (;;) {
        sh2_service();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // vTaskDelete(NULL);
}

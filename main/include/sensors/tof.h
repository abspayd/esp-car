#ifndef SENSORS_TOF_H
#define SENSORS_TOF_H

#include "vl53l7cx_api.h"

extern VL53L7CX_Configuration vl53l7cx_dev;

// ToF sensor definitions
#define VL53L7CX_INTERRUPT_GPIO CONFIG_VL53L7CX_INTERRUPT_GPIO

extern void tof_task(void *args);

#endif

#ifndef SENSORS_IMU_H
#define SENSORS_IMU_H

#include <math.h>
#define BNO085_HOST_INTN CONFIG_BNO085_INTERRUPT_GPIO
#define BNO058_ADDRESS 0x4A

extern void imu_task(void *args);

#endif

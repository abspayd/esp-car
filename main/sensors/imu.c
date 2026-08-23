#include "sensors/imu.h"
#include "BNO085.h"

void imu_task(void *args) {
    BNO085_Read_Gyro();
    BNO085_Read_Accelerometer();
    BNO085_Read_Magnetometer();
}

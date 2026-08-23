#ifndef SH2_SPI_H

#include "sh2_hal.h"

// This function initializes communications with the device.  It
// can initialize any GPIO pins and peripheral devices used to
// interface with the sensor hub.
// It should also perform a reset cycle on the sensor hub to
// ensure communications start from a known state.
extern int spi_open(sh2_Hal_t *self);

// This function completes communications with the sensor hub.
// It should put the device in reset then de-initialize any
// peripherals or hardware resources that were used.
extern void spi_close(sh2_Hal_t *self);

// This function supports reading data from the sensor hub.
// It will be called frequently to service the device.
//
// If the HAL has received a full SHTP transfer, this function
// should load the data into pBuffer, set the timestamp to the
// time the interrupt was detected, and return the non-zero length
// of data in this transfer.
//
// If the HAL has not recevied a full SHTP transfer, this function
// should return 0.
//
// Because this function is called regularly, it can be used to
// perform other housekeeping operations.  (In the case of UART
// interfacing, bytes transmitted are staggered in time and this
// function can be used to keep the transmission flowing.)
extern int spi_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);

// This function supports writing data to the sensor hub.
// It is called each time the application has a block of data to
// transfer to the device.
//
// If the device isn't ready to receive data, this function can
// return 0 without performing the transmit function.
//
// If the transmission can be started, this function needs to
// copy the data from pBuffer and return the number of bytes
// accepted.  It need not block.  The actual transmission of
// the data can continue after this function returns.
extern int spi_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);

// This function should return a 32-bit value representing a
// microsecond counter.  The count may roll over after 2^32
// microseconds.
extern uint32_t spi_getTimeUs(sh2_Hal_t *self);

extern sh2_Hal_t sh2_hal;

#endif

#include "sh2/sh2_hal.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "sh2_spi.h"

#define SPI_MOSI_GPIO CONFIG_SPI_MOSI_GPIO
#define SPI_MISO_GPIO CONFIG_SPI_MISO_GPIO
#define SPI_SCK_GPIO CONFIG_SPI_SCK_GPIO

#define BNO085_SPI_CSN_GPIO CONFIG_BNO085_CSN_GPIO
#define BNO085_RESET_GPIO CONFIG_BNO085_RESET_GPIO
#define BNO085_INTERRUPT_GPIO CONFIG_BNO085_INTERRUPT_GPIO
#define BNO085_WAKE_GPIO CONFIG_BNO085_WAKE_GPIO

static spi_bus_config_t spi_bus_cfg;
static spi_device_handle_t spi_dev_handle;

static uint8_t rx_buffer[SH2_HAL_MAX_TRANSFER_IN];
static uint32_t rx_buffer_len;

sh2_Hal_t sh2_hal = {
    .open = spi_open,
    .close = spi_close,
    .read = spi_read,
    .write = spi_write,
    .getTimeUs = spi_getTimeUs,
};

void enable_gpio_pins(void) {
    gpio_set_direction(BNO085_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO085_WAKE_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(BNO085_INTERRUPT_GPIO, GPIO_MODE_INPUT);
}

void reset_device(void) {
    gpio_set_level(BNO085_WAKE_GPIO, 1);
    gpio_set_level(BNO085_RESET_GPIO, 0);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    gpio_set_level(BNO085_RESET_GPIO, 1);

    vTaskDelay(100 / portTICK_PERIOD_MS);
}

// This function initializes communications with the device.  It
// can initialize any GPIO pins and peripheral devices used to
// interface with the sensor hub.
// It should also perform a reset cycle on the sensor hub to
// ensure communications start from a known state.
int spi_open(sh2_Hal_t *self) {

    printf("spi_open called\n");

    enable_gpio_pins();

    spi_bus_cfg = (spi_bus_config_t){
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .sclk_io_num = SPI_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_cfg, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t spi_dev_cfg = {
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 3 * 1000 * 1000,
        .spics_io_num = BNO085_SPI_CSN_GPIO,
        .mode = 3,
        .queue_size = 1,
    };

    // spi_device_handle_t bno085_handle;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &spi_dev_cfg, &spi_dev_handle));

    reset_device();

    return 0;
}

// This function completes communications with the sensor hub.
// It should put the device in reset then de-initialize any
// peripherals or hardware resources that were used.
void spi_close(sh2_Hal_t *self) { reset_device(); }

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
int spi_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us) {

    int res = 0;

    if (len >= rx_buffer_len) {
        memcpy(pBuffer, rx_buffer, rx_buffer_len);

        res = rx_buffer_len;

        rx_buffer_len = 0;

        *t_us = (uint32_t)esp_timer_get_time();
    }

    return res;
}

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
int spi_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {

    esp_err_t err = spi_device_acquire_bus(spi_dev_handle, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE("write", "Failed to acquire SPI bus: %s", esp_err_to_name(err));
        return 0;
    }

    size_t read_length = 0;
    if (SH2_HAL_MAX_TRANSFER_IN - rx_buffer_len < len) {
        read_length = SH2_HAL_MAX_TRANSFER_IN - rx_buffer_len;
    } else {
        read_length = SH2_HAL_MAX_TRANSFER_IN - rx_buffer_len - len;
    }

    if (read_length > len) {
        read_length = len;
    }
    printf("read length: %zu, length: %zu\n", read_length, len);

    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = pBuffer,
        .rx_buffer = rx_buffer + rx_buffer_len,
        .rxlength = read_length * 8,
    };

    err = spi_device_transmit(spi_dev_handle, &t);
    if (err != ESP_OK) {
        ESP_LOGE("read", "Error reading transmission: %s", esp_err_to_name(err));
        spi_device_release_bus(spi_dev_handle);
        return 0;
    }

    spi_device_release_bus(spi_dev_handle);

    return len;
}

// This function should return a 32-bit value representing a
// microsecond counter.  The count may roll over after 2^32
// microseconds.
uint32_t spi_getTimeUs(sh2_Hal_t *self) { return (uint32_t)esp_timer_get_time(); }

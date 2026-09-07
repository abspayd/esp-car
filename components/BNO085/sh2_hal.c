#include "sh2/sh2_hal.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "sh2_err.h"
#include "sh2_spi.h"
#include <ctype.h>

#define SPI_MOSI_GPIO CONFIG_SPI_MOSI_GPIO
#define SPI_MISO_GPIO CONFIG_SPI_MISO_GPIO
#define SPI_SCK_GPIO CONFIG_SPI_SCK_GPIO

#define BNO085_SPI_CSN_GPIO CONFIG_BNO085_CSN_GPIO
#define BNO085_RESET_GPIO CONFIG_BNO085_RESET_GPIO
#define BNO085_INTERRUPT_GPIO CONFIG_BNO085_INTERRUPT_GPIO
#define BNO085_WAKE_GPIO CONFIG_BNO085_WAKE_GPIO

typedef enum {
    SPI_INIT,
    SPI_DUMMY, // throwaway operation to establish proper clock state
    SPI_DFU,   // firmware update
    SPI_IDLE,
    SPI_RD_HDR,  // read header
    SPI_RD_BODY, // read body
    SPI_WRITE,
} spi_state_t;

typedef enum {
    EVENT_INTERRUPT,
    EVENT_SPI_COMPLETE,
} event_type_t;

static spi_state_t spi_state = SPI_INIT;
static volatile bool rx_ready = false;
static volatile bool is_open = false;

static spi_bus_config_t spi_bus_cfg;
static spi_device_handle_t spi_dev_handle;

static uint8_t tx_buffer[SOC_SPI_MAXIMUM_BUFFER_SIZE];
static volatile uint32_t tx_buffer_len;

static uint8_t rx_buffer[SOC_SPI_MAXIMUM_BUFFER_SIZE];
static volatile uint32_t rx_buffer_len;

static QueueHandle_t event_queue = NULL;

sh2_Hal_t sh2_hal = {
    .open = spi_open,
    .close = spi_close,
    .read = spi_read,
    .write = spi_write,
    .getTimeUs = spi_getTimeUs,
};

static void spi_dummy_op(void) {
    uint8_t dummy_rx[1];
    uint8_t dummy_tx[1];

    memset(dummy_tx, 0xAA, sizeof(dummy_tx));

    spi_transaction_t t = {
        .length = sizeof(dummy_tx) * 8,
        .tx_buffer = dummy_tx,
        .rx_buffer = dummy_rx,
    };

    printf("transmit start\n");
    esp_err_t err = spi_device_transmit(spi_dev_handle, &t);
    printf("transmit end\n");
    if (err != ESP_OK) {
        ESP_LOGE("spi_dummy_op", "Error reading transmission: %s", esp_err_to_name(err));
        return;
    }
}

static void interrupt_handler(void *arg) {
    event_type_t e = EVENT_INTERRUPT;
    xQueueSendFromISR(event_queue, &e, NULL);
}

static void spi_activate(void) {
    if (spi_state != SPI_IDLE || rx_buffer_len != 0 || rx_ready == false) {
        return;
    }

    rx_ready = false;

    // enable csn
    gpio_set_level(BNO085_SPI_CSN_GPIO, 0);

    esp_err_t err;

    err = spi_device_acquire_bus(spi_dev_handle, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE("spi_activate", "Failed to acquire SPI bus: %s", esp_err_to_name(err));
        return;
    }

    size_t len = tx_buffer_len;
    uint8_t *buf = tx_buffer;
    if (len == 0) {
        // SHTP header
        buf = (uint8_t[4]){0};
        len = 4;

        spi_state = SPI_RD_HDR;
    } else {
        spi_state = SPI_WRITE;
    }

    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = buf,
        .rx_buffer = rx_buffer,
    };

    printf("transmit start\n");
    err = spi_device_transmit(spi_dev_handle, &t);
    printf("transmit end\n");
    if (err != ESP_OK) {
        ESP_LOGE("spi_activate", "Error reading transmission: %s", esp_err_to_name(err));
        return;
    }

    return;
}

static void spi_completed(void) {
    uint16_t rx_len = (rx_buffer[0] + (rx_buffer[1] << 8)) & ~0x8000;

    if (rx_len > sizeof(rx_buffer)) {
        rx_len = sizeof(rx_buffer);
    }

    switch (spi_state) {
    case SPI_DUMMY:
        spi_state = SPI_IDLE;
        break;
    case SPI_RD_HDR:
        if (rx_len > 4) {
            spi_state = SPI_RD_BODY;

            spi_transaction_t t = {
                .length = (rx_len - 4) * 8,
                .tx_buffer = NULL,
                .rx_buffer = rx_buffer + 4,
            };

            printf("transmit start\n");
            esp_err_t err = spi_device_transmit(spi_dev_handle, &t);
            printf("transmit end\n");
            if (err != ESP_OK) {
                ESP_LOGE("spi_completed", "Error reading body: %s", esp_err_to_name(err));
                return;
            }
        } else {
            // disable csn
            gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

            rx_buffer_len = 0;
            spi_state = SPI_IDLE;

            spi_activate();

            spi_device_release_bus(spi_dev_handle);
        }

        break;
    case SPI_RD_BODY:
        // disable csn
        gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

        rx_buffer_len = rx_len;

        spi_state = SPI_IDLE;

        spi_activate();

        spi_device_release_bus(spi_dev_handle);
        break;
    case SPI_WRITE:
        // disable csn
        gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

        rx_buffer_len = (tx_buffer_len < rx_len) ? tx_buffer_len : rx_len;

        tx_buffer_len = 0;

        spi_state = SPI_IDLE;

        spi_activate();

        spi_device_release_bus(spi_dev_handle);
        break;
    default:
        break;
    }
}

static void event_listener_task(void *arg) {
    for (;;) {
        event_type_t e;
        if (xQueueReceive(event_queue, &e, portMAX_DELAY)) {
            switch (e) {
            case EVENT_INTERRUPT:
                // printf("BNO085 Interrupt detected\n");

                rx_ready = true;

                spi_activate();
                break;
            case EVENT_SPI_COMPLETE:
                if (is_open) {
                    spi_completed();
                }
                break;
            default:
                ESP_LOGE("event_listener_task", "UNKNOWN EVEVENT: %d", e);
                break;
            }
        }
    }
}

static void spi_completed_callback(spi_transaction_t *t) {
    event_type_t e = EVENT_SPI_COMPLETE;
    xQueueSend(event_queue, &e, 0);
}

void enable_gpio_pins(void) {
    gpio_set_direction(BNO085_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO085_WAKE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO085_SPI_CSN_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(BNO085_INTERRUPT_GPIO, GPIO_MODE_INPUT);

    event_queue = xQueueCreate(10, sizeof(event_type_t));
    xTaskCreate(event_listener_task, "bno085_interrupt_task", 2048, NULL, 10, NULL);

    gpio_install_isr_service(0);

    gpio_set_intr_type(BNO085_INTERRUPT_GPIO, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(BNO085_INTERRUPT_GPIO, interrupt_handler, NULL);
    gpio_intr_disable(BNO085_INTERRUPT_GPIO);
}

void reset_device(void) {
    gpio_set_level(BNO085_WAKE_GPIO, 1);
    gpio_set_level(BNO085_RESET_GPIO, 0);

    spi_state = SPI_DUMMY;
    spi_dummy_op();
    spi_state = SPI_IDLE;

    vTaskDelay(10000 / portTICK_PERIOD_MS);

    gpio_set_level(BNO085_RESET_GPIO, 1);
}

// This function initializes communications with the device.  It
// can initialize any GPIO pins and peripheral devices used to
// interface with the sensor hub.
// It should also perform a reset cycle on the sensor hub to
// ensure communications start from a known state.
int spi_open(sh2_Hal_t *self) {

    // TODO: use logic analyzer to read MOSI, MISO, SCK, and CS pins
    // try figure out why headers aren't being received

    enable_gpio_pins();

    is_open = true;

    // disable csn
    gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

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
        .post_cb = spi_completed_callback,
        .mode = 3,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &spi_dev_cfg, &spi_dev_handle));

    reset_device();

    gpio_intr_enable(BNO085_INTERRUPT_GPIO);

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    return 0;
}

// This function completes communications with the sensor hub.
// It should put the device in reset then de-initialize any
// peripherals or hardware resources that were used.
void spi_close(sh2_Hal_t *self) {
    reset_device();

    // disable csn
    gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

    spi_state = SPI_INIT;

    is_open = false;
}

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

    // printf("Buffer length: %lu, request length: %u\n", rx_buffer_len, len);

    if (rx_buffer_len > 0) {

        // printf("rx_buffer_len: %ld, len: %d\n", rx_buffer_len, len);

        if (len >= rx_buffer_len) {
            memcpy(pBuffer, rx_buffer, rx_buffer_len);

            res = rx_buffer_len;

            *t_us = (uint32_t)esp_timer_get_time();

            for (int i = 0; i < rx_buffer_len; i++) {
                if (isalnum((char)rx_buffer[i])) {
                    printf("0x%02X - %c\n", rx_buffer[i], rx_buffer[i]);
                } else {
                    printf("0x%02X\n", rx_buffer[i]);
                }
            }

            rx_buffer_len = 0;

        } else {
            rx_buffer_len = 0;
        }

        gpio_intr_disable(BNO085_INTERRUPT_GPIO);
        spi_activate();
        gpio_intr_enable(BNO085_INTERRUPT_GPIO);
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

    if (self == 0 || len > SOC_SPI_MAXIMUM_BUFFER_SIZE || (len > 0 && pBuffer == 0)) {
        return SH2_ERR_BAD_PARAM;
    }

    if (tx_buffer_len != 0) {
        return 0;
    }

    int res = 0;

    memcpy(tx_buffer, pBuffer, len);
    tx_buffer_len = len;
    res = len;

    return res;
}

// This function should return a 32-bit value representing a
// microsecond counter.  The count may roll over after 2^32
// microseconds.
uint32_t spi_getTimeUs(sh2_Hal_t *self) { return esp_timer_get_time(); }

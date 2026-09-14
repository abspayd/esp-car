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

#define INCLUDE_vTaskSuspend 1

#define SPI_MOSI_GPIO CONFIG_SPI_MOSI_GPIO
#define SPI_MISO_GPIO CONFIG_SPI_MISO_GPIO
#define SPI_SCK_GPIO CONFIG_SPI_SCK_GPIO

#define BNO085_SPI_CSN_GPIO CONFIG_BNO085_CSN_GPIO
#define BNO085_RESET_GPIO CONFIG_BNO085_RESET_GPIO
#define BNO085_INTERRUPT_GPIO CONFIG_BNO085_INTERRUPT_GPIO
#define BNO085_WAKE_GPIO CONFIG_BNO085_WAKE_GPIO

static SemaphoreHandle_t spi_transmit_lock;

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
    EVENT_CLOSE,
} event_type_t;

static spi_state_t spi_state = SPI_INIT;
static volatile bool rx_ready = false;
static volatile bool is_open = false;

static volatile uint32_t rx_timestamp_us;

static spi_bus_config_t spi_bus_cfg;
static spi_device_handle_t spi_dev_handle;

DMA_ATTR uint8_t tx_zeros[SH2_HAL_MAX_TRANSFER_IN] = {0};
DMA_ATTR uint8_t tx_buffer[SH2_HAL_MAX_TRANSFER_OUT] = {0};
DMA_ATTR uint8_t rx_buffer[SH2_HAL_MAX_TRANSFER_IN] = {0};
static volatile uint32_t tx_buffer_len;
static volatile uint32_t rx_buffer_len;

static QueueHandle_t event_queue = NULL;

static sh2_Hal_t sh2_hal;

sh2_Hal_t *sh2_hal_init(void) {

    sh2_hal.open = spi_open;
    sh2_hal.close = spi_close;
    sh2_hal.read = spi_read;
    sh2_hal.write = spi_write;
    sh2_hal.getTimeUs = spi_getTimeUs;

    return &sh2_hal;
}

static int spi_transmit(spi_device_handle_t handle, uint8_t *rx_buffer, uint8_t *tx_buffer, uint32_t len) {
    esp_err_t err;
    if (xSemaphoreTake(spi_transmit_lock, 10 / portTICK_PERIOD_MS)) {
        spi_transaction_t t = {
            .length = len * 8,
            .tx_buffer = tx_buffer,
            .rx_buffer = rx_buffer,
        };

        err = spi_device_polling_transmit(handle, &t);
        if (err != ESP_OK) {
            ESP_LOGE("spi_transmit", "SPI transmit error: %s", esp_err_to_name(err));
            return err;
        }
        xSemaphoreGive(spi_transmit_lock);
    } else {
        ESP_LOGE("spi_transmit", "Unable to take transmission lock");
        xSemaphoreGive(spi_transmit_lock);
        return 1;
    }

    return 0;
}

static void spi_dummy_op(void) {
    uint8_t dummy_rx[1];
    uint8_t dummy_tx[1];

    memset(dummy_tx, 0xAA, sizeof(dummy_tx));

    spi_transmit(spi_dev_handle, dummy_rx, dummy_tx, sizeof(dummy_tx));
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

    if (tx_buffer_len > 0) {
        spi_state = SPI_WRITE;

        spi_transmit(spi_dev_handle, rx_buffer, tx_buffer, tx_buffer_len);

        // de-assert wake
        gpio_set_level(BNO085_WAKE_GPIO, 1);
    } else {
        spi_state = SPI_RD_HDR;
        spi_transmit(spi_dev_handle, rx_buffer, tx_zeros, 4);
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

            spi_transmit(spi_dev_handle, rx_buffer + 4, tx_zeros, rx_len - 4);
        } else {
            // disable csn
            gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

            rx_buffer_len = 0;
            spi_state = SPI_IDLE;

            spi_activate();
        }

        break;
    case SPI_RD_BODY:
        // disable csn
        gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

        rx_buffer_len = rx_len;

        spi_state = SPI_IDLE;

        spi_activate();
        break;
    case SPI_WRITE:
        // disable csn
        gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

        rx_buffer_len = (tx_buffer_len < rx_len) ? tx_buffer_len : rx_len;

        spi_state = SPI_IDLE;

        tx_buffer_len = 0;

        spi_activate();
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
                rx_timestamp_us = (uint32_t)esp_timer_get_time();

                rx_ready = true;

                spi_activate();
                break;
            case EVENT_SPI_COMPLETE:
                if (is_open) {
                    spi_completed();
                }
                break;
            case EVENT_CLOSE:
                vTaskDelete(NULL);
                break;
            default:
                // Ignore unknown events
                break;
            }
        }
    }
}

static void spi_completed_callback(spi_transaction_t *t) {
    event_type_t e = EVENT_SPI_COMPLETE;
    xQueueSendFromISR(event_queue, &e, NULL);
}

void enable_gpio_pins(void) {
    gpio_set_direction(BNO085_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO085_WAKE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO085_SPI_CSN_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(BNO085_INTERRUPT_GPIO, GPIO_MODE_INPUT);

    event_queue = xQueueCreate(1, sizeof(event_type_t));
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

    enable_gpio_pins();

    is_open = true;

    rx_buffer_len = 0;
    tx_buffer_len = 0;

    spi_transmit_lock = xSemaphoreCreateBinary();
    if (spi_transmit_lock == NULL) {
        ESP_LOGE("spi_open", "Failed to create binary semaphore lock");
    } else {
        xSemaphoreGive(spi_transmit_lock);
    }

    // disable csn
    gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

    spi_bus_cfg = (spi_bus_config_t){
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .sclk_io_num = SPI_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_cfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_cfg, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t spi_dev_cfg = {
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 3 * 1000 * 1000,
        .post_cb = spi_completed_callback,
        .spics_io_num = -1,
        .mode = 3,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &spi_dev_cfg, &spi_dev_handle));

    gpio_set_level(BNO085_WAKE_GPIO, 1);
    reset_device();

    gpio_intr_enable(BNO085_INTERRUPT_GPIO);

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    return 0;
}

// This function completes communications with the sensor hub.
// It should put the device in reset then de-initialize any
// peripherals or hardware resources that were used.
void spi_close(sh2_Hal_t *self) {
    ESP_LOGI("spi_close", "Closing SPI");

    reset_device();

    // disable csn
    gpio_set_level(BNO085_SPI_CSN_GPIO, 1);

    spi_state = SPI_INIT;

    is_open = false;

    // event_type_t e = EVENT_CLOSE;
    // xQueueSend(event_queue, &e, 0);
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

    if (rx_buffer_len > 0) {

        if (len >= rx_buffer_len) {
            memcpy(pBuffer, rx_buffer, rx_buffer_len);

            res = rx_buffer_len;

            *t_us = rx_timestamp_us;

            rx_buffer_len = 0;
        } else {
            res = SH2_ERR_BAD_PARAM;
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

    if (self == 0 || len > sizeof(tx_buffer) || (len > 0 && pBuffer == 0)) {
        return SH2_ERR_BAD_PARAM;
    }

    if (tx_buffer_len != 0) {
        return 0;
    }

    int res = 0;

    memcpy(tx_buffer, pBuffer, len);
    tx_buffer_len = len;
    res = len;

    gpio_intr_disable(BNO085_INTERRUPT_GPIO);
    gpio_set_level(BNO085_WAKE_GPIO, 0);
    gpio_intr_enable(BNO085_INTERRUPT_GPIO);

    return res;
}

// This function should return a 32-bit value representing a
// microsecond counter.  The count may roll over after 2^32
// microseconds.
uint32_t spi_getTimeUs(sh2_Hal_t *self) { return esp_timer_get_time(); }

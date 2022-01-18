#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "imu.h"

/* Which SPI instance to use */
#define SPI_PORT spi0

/* AKA MOSI and MISO */
#define PIN_TX   19
#define PIN_RX   16
#define PIN_CS   17
#define PIN_SCLK 18
#define PIN_DR   20
#define PIN_RST  22
#define PIN_LED  25

/* Microseconds to stall between 16 bit transfers */
#define STALL_TIME 20

/* Which event happens when the registers are ready */
#define IRQ_LEVEL GPIO_IRQ_EDGE_RISE

void stall() {
    sleep_us(STALL_TIME);
}

void select() {
    gpio_put(PIN_CS, 0);
}

void deselect() {
    gpio_put(PIN_CS, 1);
}

void IMU_SPI_Init() {
    stdio_init_all();

    /* Use SPI at 1MHz on spi0. */
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_RX,   GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_TX,   GPIO_FUNC_SPI);

    spi_set_format(SPI_PORT, 16, 1, 1, SPI_MSB_FIRST);

    /* Chip select and reset are active-low, so we initialise them to a driven-high state */
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);

    /* Set LED on */
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 1);
}

uint16_t IMU_SPI_Transfer(uint32_t MOSI) {
    uint16_t msg = MOSI;
    uint16_t res = 0;

    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &msg, 1);
    gpio_put(PIN_CS, 1);

    sleep_us(STALL_TIME);

    gpio_put(PIN_CS, 0);
    spi_read16_blocking(SPI_PORT, 0, &res, 1);
    gpio_put(PIN_CS, 1);

    sleep_us(STALL_TIME);

    return res;
}

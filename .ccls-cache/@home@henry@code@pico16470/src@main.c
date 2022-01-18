#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include "imu.h"

/* ADI 16470 Pin/Port Defines: */
#define SPI_PORT spi0

/* Chip select / Slave select */
#define PIN_CS   17

/* Serial clock */
#define PIN_SCLK 18

/* Master out, slave in / Transmit */
#define PIN_MOSI 19

/* Slave out, master in / Receive */
#define PIN_MISO 16

/* Interrupt request / Data ready */
#define PIN_DR   20

/* Onboard LED pin */
#define LED_PIN  25

/**
  * @brief Basic IMU SPI data transfer function (protocol agnostic).
  *
  * @param MOSI The 16 bit MOSI data to transmit to the IMU
  *
  * @return The 16 bit MISO data received during the transmission
  *
  * This function wraps the SPI master HAL layer into something easily usable. All
  * SPI pass through functionality is built on this call.
 **/
uint16_t IMU_SPI_Transfer(uint32_t MOSI) {
    uint16_t src[1] = {MOSI};
    uint16_t dst[1];

    /* Drop CS */
    gpio_put(PIN_CS, 0);

    /* Transfer first 16 bits of MOSI */
    spi_write16_blocking(SPI_PORT, src, 1);

    /* Read 16 bit response into res */
    int res = spi_read16_blocking(SPI_PORT, 0, dst, 1);

    /* Bring CS high */
    gpio_put(PIN_CS, 1);

    return dst[0];
}

void loop() {
    /* Reads PROD_ID register */
    uint16_t id = IMU_SPI_Transfer(0x7200);
    sleep_ms(1000);
}

int main()
{
    stdio_init_all();

    /* SPI initialisation. This example will use SPI at 1MHz. */
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    spi_set_format(SPI_PORT, 16, 1, 1, SPI_MSB_FIRST);

    /* Chip select is active-low, so we'll initialise it to a driven-high state */
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    while (true) {
        loop();
    }
}

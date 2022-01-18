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

/* Master out, slave in (TX) */
#define PIN_MOSI 19

/* Slave out, master in (RX) */
#define PIN_MISO 16

/* Interrupt request / Data ready */
#define PIN_DR   20

/* Onboard LED pin */
#define LED_PIN  25

void loop() {

}

int main()
{
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCLK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    while (true) {
        loop();
    }
}

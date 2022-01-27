#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
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

/* Half-words to read from RX during a burst read */
#define BURST_LEN 10

/* Half-word to trigger IMU burst read */
#define BURST_START 0x6800

/* Which event happens when the registers are ready */
#define IRQ_LEVEL GPIO_IRQ_EDGE_RISE

static uint16_t imu_write_buf[] = {BURST_START, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0,           0, 0, 0, 0, 0, 0, 0};

dma_channel_config dma_rx_config;
uint dma_rx;

dma_channel_config dma_tx_config;
uint dma_tx;

void SPI_Select() {
    gpio_put(PIN_CS, 0);
}

void SPI_Deselect() {
    gpio_put(PIN_CS, 1);
}

void IMU_SPI_Init() {
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

    dma_tx = dma_claim_unused_channel(true);
    dma_rx = dma_claim_unused_channel(true);

    // We set the outbound DMA to transfer from a memory buffer to the SPI
    // transmit FIFO paced by the SPI TX FIFO DREQ The default is for the read
    // address to increment every element (in this case 2 bytes - DMA_SIZE_16)
    // and for the write address to remain unchanged.
    dma_tx_config = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_16);
    channel_config_set_dreq(&dma_tx_config, spi_get_dreq(SPI_PORT, true));

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory
    // buffer paced by the SPI RX FIFO DREQ We configure the read address to
    // remain unchanged for each element, but the write address to increment (so
    // data is written throughout the buffer)
    dma_rx_config = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&dma_rx_config, DMA_SIZE_16);
    channel_config_set_dreq(&dma_rx_config, spi_get_dreq(SPI_PORT, false));
    channel_config_set_read_increment(&dma_rx_config, false);
    channel_config_set_write_increment(&dma_rx_config, true);

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dma_rx, true);

    // Call IMU_Finish_Burst() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, IMU_Finish_Burst);
    irq_set_enabled(DMA_IRQ_0, true);
}

uint16_t IMU_SPI_Transfer(uint32_t MOSI) {
    uint16_t msg = MOSI;
    uint16_t res = 0;

    SPI_Select();
    spi_write16_blocking(SPI_PORT, &msg, 1);
    SPI_Deselect();

    sleep_us(STALL_TIME);

    SPI_Select();
    spi_read16_blocking(SPI_PORT, 0, &res, 1);
    SPI_Deselect();

    sleep_us(STALL_TIME);

    return res;
}

uint16_t IMU_Read_Register(uint8_t RegAddr) {
    /* Write a 0 the R/W pin, then the address */
    uint16_t msg = (((uint16_t)RegAddr) << 8) & 0x7FFF;

    return IMU_SPI_Transfer(msg);
}

/* NOTE: each register has 2 bytes that need to be written to */
uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue) {
    /* Write a 1 to the R/W pin, then the address and new value */
    uint16_t msg = (0x8000 | (((uint16_t)RegAddr) << 8) | RegValue);

    return IMU_SPI_Transfer(msg);
}

/* Perform burst read with the ordinary SPI library and block until done */
void IMU_Read_Burst(uint16_t *buf) {
    SPI_Select();
    uint16_t msg = BURST_START;
    spi_write16_blocking(SPI_PORT, &msg, 1);
    spi_read16_blocking(SPI_PORT, 0x00, buf, BURST_LEN);
    SPI_Deselect();
}

/* Start DMA channels to begin transfering memory from the IMU to buffers */
void IMU_Start_Burst(uint16_t *buf) {
    /* Drop CS */
    SPI_Select();

    dma_channel_configure(dma_tx, &dma_tx_config,
                          &spi_get_hw(SPI_PORT)->dr,  // write address
                          &imu_write_buf,             // read address
                          BURST_LEN,                  // half-words to transfer
                          false);                     // don't start yet
    dma_channel_configure(dma_rx, &dma_rx_config,
                          &buf,                       // write address
                          &spi_get_hw(SPI_PORT)->dr,  // read address
                          BURST_LEN,                  // half-words to transfer
                          false);                     // don't start yet
    /* Start DMA Transfer */
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
}

/* Called when burst read finishes. Flashes the LED so I know it was called. */
void IMU_Finish_Burst() {
    /* Bring CS high */
    SPI_Deselect();
    /* Turn on LED */
    gpio_put(PIN_LED, 1);
}

void IMU_Reset() {
  gpio_put(PIN_RST, 0);  // Active low
  sleep_ms(10);
  gpio_put(PIN_RST, 1);  // Active low
  sleep_ms(310);
}

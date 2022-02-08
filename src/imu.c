#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "imu.h"

/* Which SPI instance to use */
#define SPI_PORT spi0

#define PIN_TX   19
#define PIN_RX   16
#define PIN_CS   17
#define PIN_SCLK 18
#define PIN_DR   20
#define PIN_RST  22
#define PIN_LED  25

/* Microseconds to stall between 16 bit transfers */
#define STALL_TIME 20

/* Half-words to read from RX during a burst read. One more than the response
 * length because the first read happens while sending the burst read signal */
#define BURST_LEN 11

/* DR pin goes from low to high when registers are updated */
#define IRQ_LEVEL GPIO_IRQ_EDGE_RISE

void IMU_DMA_Finish_Burst();

static const uint16_t start_burst_read = 0x6800;
static const uint16_t imu_write_buf[BURST_LEN] = {start_burst_read, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static dma_channel_config dma_rx_config;
static uint dma_rx;

static dma_channel_config dma_tx_config;
static uint dma_tx;

static inline void SPI_Select() {
    gpio_put(PIN_CS, 0);
}

static inline void SPI_Deselect() {
    gpio_put(PIN_CS, 1);
}

void IMU_SPI_Init() {
    /* Use SPI at 1MHz */
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

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);

    dma_tx = dma_claim_unused_channel(true);
    dma_rx = dma_claim_unused_channel(true);

    /* We set the outbound DMA to transfer from a memory buffer to the SPI
     * transmit FIFO paced by the SPI TX FIFO DREQ The default is for the read
     * address to increment every element (in this case 2 bytes - DMA_SIZE_16)
     * and for the write address to remain unchanged. */
    dma_tx_config = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_16);
    channel_config_set_dreq(&dma_tx_config, spi_get_dreq(SPI_PORT, true));
    channel_config_set_read_increment(&dma_tx_config, true);
    channel_config_set_write_increment(&dma_tx_config, false);

    /* We set the inbound DMA to transfer from the SPI receive FIFO to a memory
     * buffer paced by the SPI RX FIFO DREQ We configure the read address to
     * remain unchanged for each element, but the write address to increment (so
     * data is written throughout the buffer) */
    dma_rx_config = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&dma_rx_config, DMA_SIZE_16);
    channel_config_set_dreq(&dma_rx_config, spi_get_dreq(SPI_PORT, false));
    channel_config_set_read_increment(&dma_rx_config, false);
    channel_config_set_write_increment(&dma_rx_config, true);

    /* Tell the DMA to raise IRQ line 0 when the channel finishes a block */
    dma_channel_set_irq0_enabled(dma_rx, true);

    /* Call IMU_Finish_Burst() when DMA IRQ 0 is asserted */
    irq_set_exclusive_handler(DMA_IRQ_0, IMU_DMA_Finish_Burst);
    irq_set_enabled(DMA_IRQ_0, true);
}

uint16_t IMU_SPI_Transfer(uint16_t msg) {
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
    /* Write a 0 to the R/W bit, the address to the next seven bits,
     * and don't care about the last 8 bits */
    uint16_t msg = (((uint16_t)RegAddr) << 8) & 0x7FFF;

    return IMU_SPI_Transfer(msg);
}

/* NOTE: each register has 2 bytes that need to be written to */
uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue) {
    /* Write a 1 to the R/W bit, the address to the next seven bits,
     * and the new value to the last 8 bits */
    uint16_t msg = (0x8000 | (((uint16_t)RegAddr) << 8) | RegValue);

    return IMU_SPI_Transfer(msg);
}

void IMU_Burst_Read_Blocking(uint16_t *buf) {
    SPI_Select();
    spi_write16_blocking(SPI_PORT, &start_burst_read, 1);
    spi_read16_blocking(SPI_PORT, 0x00, buf, BURST_LEN);
    SPI_Deselect();
}

void IMU_Reset() {
    /* Reset pin is active low */
    gpio_put(PIN_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_RST, 1);
    sleep_ms(310);
}

/* Start DMA channels to begin transferring memory from the IMU to buffers */
void IMU_DMA_Burst_Read(uint16_t *buf) {
    SPI_Select();

    dma_channel_configure(dma_tx, &dma_tx_config,
                          &spi_get_hw(SPI_PORT)->dr,  /* write address */
                          imu_write_buf,              /* read address */
                          BURST_LEN,                  /* half-words to transfer */
                          false);                     /* don't start yet */
    dma_channel_configure(dma_rx, &dma_rx_config,
                          buf,                        /* write address */
                          &spi_get_hw(SPI_PORT)->dr,  /* read address */
                          BURST_LEN,                  /* half-words to transfer */
                          false);                     /* don't start yet */

    /* Start both channels */
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
}

void IMU_DMA_Finish_Burst() {
    SPI_Deselect();

    /* Clear the interrupt */
    dma_hw->ints0 = 1u << dma_rx;
}

void IMU_DMA_Burst_Wait() {
    dma_channel_wait_for_finish_blocking(dma_rx);
    if (dma_channel_is_busy(dma_tx)) {
        panic("RX completed before TX\r\n");
    }
}

inline void IMU_Hook_DR(void *callback) {
    /* Set interrupt request on GPIO pin for DR */
    gpio_set_irq_enabled_with_callback(PIN_DR, IRQ_LEVEL, 1, callback);
}

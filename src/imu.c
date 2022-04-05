#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "imu.h"
#include "reg.h"
#include "isr.h"

/* Which SPI instance to use */
#define SPI_PORT spi0

#define PIN_RX   4
#define PIN_CS   1
#define PIN_SCLK 2
#define PIN_TX   3
#define PIN_RST  6

/* Microseconds to stall after chip select. Seems like it should be 16 */
static uint32_t stall_time = 20;

void IMU_DMA_Finish_Burst();

static dma_channel_config dma_rx_config;
static uint dma_rx;

static dma_channel_config dma_tx_config;
static uint dma_tx;

static bool dma_done = false;

static inline void spi_select() {
    gpio_put(PIN_CS, 0);
}

static inline void spi_deselect() {
    gpio_put(PIN_CS, 1);
}

static void dma_rx_callback()
{
    /* Clear the interrupt */
    dma_hw->ints0 = 1u << dma_rx;

    /* Clear interrupt enable */
    irq_clear(DMA_IRQ_0);

    /* Check if both interrupts complete */
    if(dma_done == 0)
    {
        dma_done = 1;
    }
    else
    {
        /* Both are done */
        IMU_DMA_Finish_Burst();
    }
}

static void dma_tx_callback()
{
    /* Clear the interrupt */
    dma_hw->ints1 = 1u << dma_tx;

    /* Clear interrupt enable */
    irq_clear(DMA_IRQ_1);

    /* Check if both interrupts complete */
    if(dma_done == 0)
    {
        dma_done = 1;
    }
    else
    {
        /* Both are done */
        IMU_DMA_Finish_Burst();
    }
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
    /* Call dma_rx_callback when DMA IRQ 0 is asserted */
    irq_set_exclusive_handler(DMA_IRQ_0, dma_rx_callback);
    irq_set_enabled(DMA_IRQ_0, true);

    /* Tell the DMA to raise IRQ line 1 when the channel finishes a block */
    dma_channel_set_irq1_enabled(dma_tx, true);
    /* Call dma_tx_callback when DMA IRQ 1 is asserted */
    irq_set_exclusive_handler(DMA_IRQ_1, dma_tx_callback);
    irq_set_enabled(DMA_IRQ_1, true);

}

uint16_t IMU_SPI_Transfer(uint16_t msg) {
    uint16_t res = 0;

    spi_select();
    spi_write16_blocking(SPI_PORT, &msg, 1);
    spi_deselect();

    sleep_us(stall_time);

    spi_select();
    spi_read16_blocking(SPI_PORT, 0, &res, 1);
    spi_deselect();

    sleep_us(stall_time);

    return res;
}

uint16_t IMU_Read_Register(uint8_t RegAddr) {
    /* Write a 0 to the R/W bit, the address to the next seven bits,
     * and don't care about the last 8 bits */
    uint16_t msg = (((uint16_t)RegAddr) << 8) & 0x7FFF;

    return IMU_SPI_Transfer(msg);
}

uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue) {
    /* Write a 1 to the R/W bit, the address to the next seven bits,
     * and the new value to the last 8 bits */
    uint16_t msg = (0x8000 | (((uint16_t)RegAddr) << 8) | RegValue);

    return IMU_SPI_Transfer(msg);
}

void IMU_Reset() {
    /* Reset pin is active low */
    gpio_put(PIN_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_RST, 1);
    sleep_ms(310);
}

/* Start DMA channels to begin transferring memory from the IMU to buffers */
void IMU_DMA_Start_Burst(uint8_t *buf) {
    spi_select();

    dma_channel_configure(dma_tx, &dma_tx_config,
                          &spi_get_hw(SPI_PORT)->dr, /* write address */
                          &g_regs[BUF_WRITE_0_REG],  /* read address */
                          g_regs[BUF_LEN_REG] / 2,   /* 16 bit words to transfer */
                          false);                    /* don't start yet */
    dma_channel_configure(dma_rx, &dma_rx_config,
                          buf,                       /* write address */
                          &spi_get_hw(SPI_PORT)->dr, /* read address */
                          g_regs[BUF_LEN_REG] / 2,   /* 16 bit words to transfer */
                          false);                    /* don't start yet */

    /* Start both channels */
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
}

/* Cleanup after DMA */
void IMU_DMA_Finish_Burst() {
    spi_deselect();
    dma_done = false;
    ISR_Finish_IMU_Burst();
}

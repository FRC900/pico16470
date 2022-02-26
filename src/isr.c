#include "isr.h"
#include "usb.h"
#include "reg.h"
#include "imu.h"
#include "timer.h"
#include "buffer.h"

static const char NoIMUBurstError[] = "Unimplemented: data capture without IMU_BURST enabled \r\n";

/* Track if there is currently a capture in progress */
volatile uint32_t g_captureInProgress = 0u;

/* Pointer to buffer element which is being populated */
static uint8_t* BufferElementHandle;

/* Pointer to buffer data signature within buffer element which is being populated */
static uint16_t* BufferSigHandle;

/* Track number of words captured within current buffer entry */
static volatile uint32_t WordsCaptured;

/* Buffer signature */
static uint32_t BufferSignature;

void ISR_Start_IMU_Burst()
{
    /* If capture in progress then set error flag and exit */
    if(g_captureInProgress)
    {
        g_captureInProgress = 0;
        g_regs[STATUS_0_REG] |= STATUS_OVERRUN;
        g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
        return;
    }

    /* If buffer element cannot be added then exit */
    if(!Buffer_Can_Add_Element())
        return;

    uint32_t SampleTimestampUs = Timer_Get_Microsecond_Timestamp();
    uint32_t SampleTimestampS = Timer_Get_PPS_Timestamp();

    /* Get element handle */
    BufferElementHandle = Buffer_Add_Element();

    /* Add timestamp to buffer */
    *(uint32_t *) BufferElementHandle = SampleTimestampS;
    *(uint32_t *) (BufferElementHandle + 4) = SampleTimestampUs;

    /* Set signature to timestamp value initially */
    BufferSignature = SampleTimestampS & 0xFFFF;
    BufferSignature += (SampleTimestampS >> 16);
    BufferSignature += SampleTimestampUs & 0xFFFF;
    BufferSignature += (SampleTimestampUs >> 16);

    /* Set buffer signature handle */
    BufferSigHandle = (uint16_t *) (BufferElementHandle + 8);
    *(BufferElementHandle + 14) = 0xFF;

    /* Offset buffer element handle by 10 bytes (timestamp + sig) */
    BufferElementHandle += 10;

    /* Set flag indicating capture is running */
    g_captureInProgress = 1;

    if(g_regs[BUF_CONFIG_REG] & BUF_CFG_IMU_BURST)
    {
        IMU_DMA_Start_Burst(BufferElementHandle);
    }
    else
    {
        /* This mode is for generating your own readout of the IMU instead of a
         * burst read, by specifying registers to read individually instead of
         * reading and writing simultaneously. We don't need this now, so it's
         * not implemented. */
        USB_Tx_Handler(NoIMUBurstError, sizeof(NoIMUBurstError) - 1);
    }
}

/**
  * @brief Cleans up an IMU burst data read
  *
  * @return void
  *
  * This function is called once the SPI1 Tx and Rx DMA interrupts
  * have both fired, or a single error interrupt has been generated.
  * SPI1 uses DMA1, channel 2/3. These channels have lower priority
  * than the user SPI DMA channels, which also use DMA peripheral 1.
  *
  * This function brings CS high, calculates the buffer signature,
  * and updates the buffer count / capture state variables. Before
  * this function is called, DMA interrupts should be disabled (by
  * their respective ISR's)
  */
void ISR_Finish_IMU_Burst()
{
    /* Build buffer signature */
    uint16_t *RxData = (uint16_t *) BufferElementHandle;

    /* Save signature to buffer entry */
    BufferSigHandle[0] = BufferSignature;

    /* Update buffer count regs with new count */
    g_regs[BUF_CNT_0_REG] = g_bufCount;
    g_regs[BUF_CNT_1_REG] = g_regs[BUF_CNT_0_REG];

    /* Mark capture as done */
    g_captureInProgress = 0;
}

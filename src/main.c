#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include "imu.h"
#include "usb.h"
#include "timer.h"
#include "isr.h"
#include "buffer.h"
#include "reg.h"
#include "data_capture.h"
#include "script.h"

#define FIRM_REV   0x6C
#define FIRM_DM    0x6E
#define FIRM_Y     0x70
#define PROD_ID    0x72
#define SERIAL_NUM 0x74
#define GLOB_CMD   0x68
#define DIAG_STAT  0x02
#define FILT_CTRL  0x5C
#define DEC_RATE   0x64

/* Byte to run a sensor self-check when sent to GLOB_CMD */
#define SELF_TEST  1u << 2

#define STATE_CHECK_FLAGS  0
#define STATE_CHECK_PPS    1
#define STATE_READ_ADC     2
#define STATE_CHECK_USB    3
#define STATE_CHECK_STREAM 4
#define STATE_STEP_SCRIPT  5

/** Track the cyclic executive state */
static uint32_t state;

int main()
{
    /* Sleep for 5s while user connects to TTY */
    sleep_ms(1000 * 5);

    stdio_init_all();

    IMU_SPI_Init();
    Timer_Init();
    Buffer_Reset();

    while (true) {
        IMU_Reset();
        uint16_t prod_id = IMU_Read_Register(0x72);
        if (prod_id == 0x4256) {
            printf("Got expected PROD_ID: 0x%04x\r\n", prod_id);
            break;
        } else {
            printf("Got unexpected PROD_ID: 0x%04x\r\n", prod_id);
        }
    }

    uint16_t firmware_revision = IMU_Read_Register(FIRM_REV);
    uint16_t firmware_day_month = IMU_Read_Register(FIRM_DM);
    uint16_t firmware_year = IMU_Read_Register(FIRM_Y);
    uint16_t serial_number = IMU_Read_Register(SERIAL_NUM);

    printf("Firmware revision: 0x%04x\r\n",  firmware_revision);
    printf("Firmware day month: 0x%04x\r\n", firmware_day_month);
    printf("Firmware year: 0x%04x\r\n",      firmware_year);
    printf("Serial number: 0x%04x\r\n",      serial_number);

    while (true) {
        IMU_Write_Register(GLOB_CMD, SELF_TEST);
        sleep_ms(24);
        uint16_t diagnostic = IMU_Read_Register(DIAG_STAT);
        if (diagnostic != 0) {
            printf("Diagnostic failure: 0x%04x\r\n", diagnostic);
        } else {
            break;
        }
    }

    IMU_Write_Register(FILT_CTRL, 0); /* No filtering */
    IMU_Write_Register(DEC_RATE, 0); /* No decimation */

    sleep_us(200);

    /* Test buffer, ISR, and IMU */
    /* g_regs[BUF_CONFIG_REG] |= BUF_CFG_IMU_BURST;
     * (write 0 FD)
     * (write 2 2)
     * g_regs[BUF_WRITE_0_REG] = 0x6800;
     * (write 0 FE)
     * (write 12 00)
     * (write 13 68) */

    while (true) {
        /* Process buf dequeue every loop iteration (high priority) */
        if(g_update_flags & DEQUEUE_BUF_FLAG)
        {
            g_update_flags &= ~DEQUEUE_BUF_FLAG;
            Reg_Buf_Dequeue_To_Outputs();
        }

        /* TODO: Set up watchdog and feed here */

        switch(state)
        {
        case STATE_CHECK_FLAGS:
            /* Handle capture disable */
            if(g_update_flags & DISABLE_CAPTURE_FLAG)
                {
                    g_update_flags &= ~DISABLE_CAPTURE_FLAG;
                    /* Make sure both can't be set */
                    g_update_flags &= ~(ENABLE_CAPTURE_FLAG);
                    Data_Capture_Disable();
                }
            /* Handle capture enable */
            else if(g_update_flags & ENABLE_CAPTURE_FLAG)
                {
                    g_update_flags &= ~ENABLE_CAPTURE_FLAG;
                    Data_Capture_Enable();
                }
            /* Handle user commands */
            else if(g_update_flags & USER_COMMAND_FLAG)
                {
                    g_update_flags &= ~USER_COMMAND_FLAG;
                    /* TODO: Determine if user commands are necessary */
                }
            /* Handle capture DIO output config change */
            else if(g_update_flags & DIO_OUTPUT_CONFIG_FLAG)
                {
                    g_update_flags &= ~DIO_OUTPUT_CONFIG_FLAG;
                    /* TODO: Determine if output interrupts are necessary */
                }
            /* Handle change to IMU SPI config */
            else if(g_update_flags & IMU_SPI_CONFIG_FLAG)
                {
                    g_update_flags &= ~IMU_SPI_CONFIG_FLAG;
                    /* TODO: Determine if updating SPI timing is necessary */
                }
            /* Advance to next state */
            state = STATE_CHECK_PPS;
            break;
        case STATE_CHECK_PPS:
            /* Check that PPS isn't unlocked */
            Timer_Check_PPS_Unlock();
            /* Advance to next state */
            state = STATE_READ_ADC;
            break;
        case STATE_READ_ADC:
            /* Update ADC state machine */
            /* TODO: Determine if temperature monitoring is necessary */
            /* Advance to next state */
            state = STATE_CHECK_USB;
            break;
        case STATE_CHECK_USB:
            /* Handle any USB command line Rx activity */
            USB_Rx_Handler();
            /* Advance to next state */
            state = STATE_CHECK_STREAM;
            break;
        case STATE_CHECK_STREAM:
            /* Check stream status for CLI */
            Script_Check_Stream();
        default:
            /* Go back to first state */
            state = STATE_CHECK_FLAGS;
            break;
        }
    }
}

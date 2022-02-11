#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include "imu.h"
#include "usb.h"

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

static uint16_t buf[11] = {0};

void data_ready(uint gpio, uint32_t events) {
    IMU_DMA_Burst_Read(buf);
    IMU_DMA_Burst_Wait();
    for (int i = 0; i < 11; i++) {
        printf("0x%04x ", buf[i]);
    }
    puts("\r\n");
}

int main()
{
    /* Sleep for 15s while user connects to TTY */
    sleep_ms(1000 * 5);

    stdio_init_all();

    IMU_SPI_Init();

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

    //IMU_Hook_DR(&data_ready);

    while (true) {
        USB_Rx_Handler();
    }
}
